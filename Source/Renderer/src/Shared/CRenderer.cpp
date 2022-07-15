#include <Renderer/Shared/CRenderer.h>
#include <Renderer/Shared/MeshProxy.h>
#include <Renderer/Shared/MaterialProxy.h>
#include <Renderer/Shared/RenderObjectProxy.h>
#include <Renderer/Shared/ResourceAllocationPoolImpl.h>
#include <Renderer/Shared/Utility/CoreRenderResourcesUtility.h>
#include <Renderer/Shared/Utility/ShaderLoader.h>
#include <Renderer/Shared/RenderPipeline/RenderPipelineManager.h>
#include <Renderer/Shared/Utility/RenderAPIAbstractionUtility.h>
#include <Renderer/Shared/BindlessTextureManager.h>
#include <Renderer/Shared/BindlessBufferManager.h>
#include <Renderer/Shared/RendererVarsList.h>

#define MAX_BINDLESS_TEXTURES_COUNT 16384
#define MAX_BINDLESS_BUFFERS_COUNT 16384



namespace YAPT
{
	Renderer* createRenderer()
	{
		return new CRenderer;
	}
	void destroyRenderer(Renderer* renderer)
	{
		delete renderer;
	}

	CRenderer::CRenderer() :
		m_gfxHandle(YAPT_NULL_HANDLE),
		m_swapChain(YAPT_NULL_HANDLE),
		m_renderPipelineMngr(nullptr),
		m_coreResourcesUtility(nullptr),
		m_shaderLoader(nullptr),
		m_cacheProvider(nullptr),
		m_renderWorkPending(false),
		m_shutDownRequested(false),
		m_currentRenderView(nullptr),
		m_frameDeltaInSeconds(0.f)
	{

	}
	CRenderer::~CRenderer()
	{
		deinit();
	}

	//ICRenderer
	void CRenderer::prepare()
	{
		Gfx::prepare(m_gfxHandle);
	}
	void CRenderer::render(const RenderParameters& renderParams)
	{
		waitForRenderThreadIdle();

		//replicate changes and allow caller to continue, while continuing with the actual rendering work
		m_meshMngr->replicateChanges();
		m_materialMngr->replicateChanges();
		m_renderObjectManager->replicateChanges();
		{
			m_hasViewMoved = m_currentRenderView->getView() != renderParams.view || m_currentRenderView->getProjection() != renderParams.projection;


			m_currentRenderView->setView(renderParams.view);
			m_currentRenderView->setProjection(renderParams.projection);
			m_currentRenderView->setNearFar(renderParams.nearFar);


			m_frameDeltaInSeconds = renderParams.frameDeltaInSeconds;
		}
		
		m_rendererConfig.commitChanges();

		Gfx::renderBegin(m_gfxHandle);
		m_textureManager->flush();
		m_bufferManager->flush();

		{
			std::unique_lock<std::mutex> lock(m_renderWorkerMutex);
			m_renderWorkPending = true;
		}

		m_renderWorkerCondition.notify_one();
	}


	void CRenderer::waitForRenderThreadIdle()
	{
		std::unique_lock<std::mutex> lock(m_renderWorkerMutex);
		if (m_renderWorkPending)
		{
			m_renderWorkerCondition.wait(lock, [this] {return !m_renderWorkPending; });
		}

	}

	bool CRenderer::initialize(const RendererInitializeConfig& config)
	{

		assert(m_gfxHandle == YAPT_NULL_HANDLE);

		GfxApiInitConfig init;
		init.pipelineLength = getPipelineLength();
		init.renderSurfaceHandle = config.renderSurfaceHandle;

		m_gfxHandle = Gfx::createGfxApiHandle(init);

		m_frameIndex = 0;

		m_cacheProvider = config.cache;

		if (m_gfxHandle == YAPT_NULL_HANDLE)
		{
			return false;
		}
		
		initVariables();
		m_threadPool.init(8);
		//start render thread
		m_renderWorkerThread = std::thread(CRenderer::renderLoopEntry, this);

		m_coreResourcesUtility = new CoreRenderResourcesUtility(this);

		m_shaderLoader = new ShaderLoader(m_gfxHandle);

		

		m_meshMngr = new MeshManager(getGfxHandle());
		m_materialMngr = new MaterialManager;
		m_renderObjectManager = new RenderObjectManager(getGfxHandle());
		m_textureManager = new BindlessTextureManager(this);
		m_textureManager->init(MAX_BINDLESS_TEXTURES_COUNT);
		m_bufferManager = new BindlessBufferManager(this);
		m_bufferManager->init(MAX_BINDLESS_BUFFERS_COUNT);

		m_currentRenderView = new RenderView(m_gfxHandle);

		RenderPipeline::InitializeContext initContext;
		initContext.renderer = this;

		m_renderPipelineMngr = new RenderPipelineManager();
		m_renderPipelineMngr->initialize(initContext);

		return true;
	}

	void CRenderer::deinit()
	{
		m_threadPool.deinit();
		//First shutdown the (highlevel) renderthread
		{
			std::unique_lock<std::mutex> lock(m_renderWorkerMutex);
			m_shutDownRequested = true;
			m_renderWorkerCondition.notify_all();
		}
		
		m_renderWorkerThread.join();

		if (m_renderPipelineMngr)
		{
			m_renderPipelineMngr->shutdown();
			delete m_renderPipelineMngr;
			m_renderPipelineMngr = nullptr;
		}

 		m_rendererConfig.clear();

		m_renderObjectManager->replicateChanges();
		m_meshMngr->replicateChanges();
		m_materialMngr->replicateChanges();
		
		delete m_currentRenderView;
		m_currentRenderView = nullptr;
		delete m_renderObjectManager;
		m_renderObjectManager = nullptr;
		delete m_meshMngr;
		m_meshMngr = nullptr;
		delete m_materialMngr;
		m_materialMngr = nullptr;
		

		

		if (m_coreResourcesUtility)
		{
			delete m_coreResourcesUtility;
			m_coreResourcesUtility = nullptr;
		}

		delete m_textureManager;
		delete m_bufferManager;

		if (m_gfxHandle != YAPT_NULL_HANDLE)
		{
			Gfx::waitForDeviceIdle(m_gfxHandle);

			if (m_swapChain != YAPT_NULL_HANDLE)
			{
				Gfx::destroySwapChain(m_gfxHandle, m_swapChain);
			}

			Gfx::destroyGfxApiHandle(m_gfxHandle);
		}

		

	}

	bool CRenderer::setRenderOutputToSurface(const WindowSurfaceDefinition& windowSurface)
	{
		resetRenderOutput();
		m_swapChain = Gfx::createSwapChain(m_gfxHandle, windowSurface);
		return m_swapChain != YAPT_NULL_HANDLE;
	}
	void CRenderer::resetRenderOutput()
	{
		waitForRenderThreadIdle();
		Gfx::waitForDeviceIdle(m_gfxHandle);
		if (m_swapChain != YAPT_NULL_HANDLE)
		{
			Gfx::destroySwapChain(m_gfxHandle, m_swapChain);
			m_swapChain = YAPT_NULL_HANDLE;
		}
	}
	ResourceAllocationPool* CRenderer::createResourceAllocationPool()
	{
		return new ResourceAllocationPoolImpl(this);
	}

	Mesh* CRenderer::createMesh(const VertexBufferLayout* layouts, size_t numberOfVertexBufferLayouts, size_t vertexCount)
	{
		return m_meshMngr->createMesh(layouts, numberOfVertexBufferLayouts, vertexCount);
	}
	Material* CRenderer::createMaterial()
	{
		return m_materialMngr->createMaterial();
	}
	RenderObject* CRenderer::createRenderObject()
	{
		return m_renderObjectManager->createRenderObject();
	}

	void CRenderer::textureToBeCreated(TextureDesc& d)
	{
		m_textureManager->textureToBeCreated(d);
	}

	void CRenderer::textureCreated(TextureImpl* t)
	{
		m_textureManager->textureCreated(t);
	}

	void CRenderer::textureReleased(TextureImpl* t)
	{
		m_textureManager->textureReleased(t);
	}

	void CRenderer::bufferToBeCreated(BufferDesc& d)
	{
		m_bufferManager->bufferToBeCreated(d);
	}

	void CRenderer::bufferCreated(BufferImpl* t)
	{
		m_bufferManager->bufferCreated(t);
	}

	void CRenderer::bufferReleased(BufferImpl* t)
	{
		m_bufferManager->bufferReleased(t);
	}

	RendererConfiguration* CRenderer::getRendererConfiguration()
	{
		return &m_rendererConfig;
	}

	void CRenderer::renderLoop()
	{
		while (true)
		{
			{
				std::unique_lock<std::mutex> lock(m_renderWorkerMutex);
				m_renderWorkerCondition.wait(lock, [this] {return m_renderWorkPending || m_shutDownRequested; });

				if (m_shutDownRequested)
				{
					//wait for the queue to be empty
					Gfx::waitForDeviceIdle(m_gfxHandle);
					return;
				}
			}

			executeFrame();

			{
				std::unique_lock<std::mutex> lock(m_renderWorkerMutex);
				m_renderWorkPending = false;
			}

			m_renderWorkerCondition.notify_one();
		}

	}

	void CRenderer::executeFrame()
	{

		m_renderObjectManager->updatePerObjectGPUData();

		if (m_renderPipelineMngr)
		{
			glm::ivec2 res = m_rendererConfig.getRendererVarValueInternal<ivec2p>(RVARNAME_RENDER_RESOLUTION);

			//calculate mvp for main view
			m_currentRenderView->issueViewDependantRenderObjectJobs(getThreadPool(), *m_renderObjectManager);

			RenderPipeline::PrepareContext prepareContext;
			prepareContext.swapChain = m_swapChain;
			prepareContext.renderWidth = res.x;
			prepareContext.renderHeight = res.y;
			m_renderPipelineMngr->prepare(prepareContext);

			//mvp needs to be ready before update calls
			getThreadPool().waitForAllTasksCompleted();
			m_currentRenderView->updatePerPerViewObjectGPUData();

			RenderPipeline::UpdateContext updateContext;
			updateContext.updateTasksPool = &getThreadPool();
			m_renderPipelineMngr->update(updateContext);

			updateContext.updateTasksPool->waitForAllTasksCompleted();
		}

		Gfx::executeBegin(m_gfxHandle);
		if (m_renderPipelineMngr)
		{
			m_renderPipelineMngr->execute();
		}
		Gfx::executeEnd(m_gfxHandle);
	}


	void CRenderer::renderLoopEntry(CRenderer* CRenderer)
	{
		CRenderer->renderLoop();
	}

	void CRenderer::initVariables()
	{
		initRVars(&m_rendererConfig);
	}


}