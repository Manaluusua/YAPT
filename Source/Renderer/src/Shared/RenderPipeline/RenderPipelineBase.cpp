#include <Renderer/Shared/RenderPipeline/RenderPipelineBase.h>
#include <Renderer/Shared/RenderPipeline/RenderStage.h>
#include <Renderer/Shared/Utility/RenderResourcesPool.h>
#include <Renderer/Shared/CRenderer.h>
#include <algorithm>

namespace YAPT
{
	RenderPipelineBase::RenderPipelineBase()
		:m_gfxHandle(YAPT_NULL_HANDLE),
		
		m_graph(nullptr),
		m_renderer(nullptr),
		m_renderResolutionWidth(0),
		m_renderResolutionHeight(0),
		m_renderResolutionDependantResources(nullptr),
		m_renderGraphLifetimeResources(nullptr),
		m_firstPrepareAfterInit(false)
	{

	}
	RenderPipelineBase::~RenderPipelineBase()
	{

	}


	void RenderPipelineBase::initialize(const InitializeContext& cntx)
	{
		m_renderer = cntx.renderer;
		m_gfxHandle = m_renderer->getGfxHandle();
		m_graph = Gfx::createRenderGraph(m_renderer->getGfxHandle());
		m_renderResolutionDependantResources = new RenderResourcesPool(m_renderer->getGfxHandle());
		m_renderGraphLifetimeResources = new RenderResourcesPool(m_renderer->getGfxHandle());
		setupRenderPipeline();

		m_firstPrepareAfterInit = true;


		RenderStage::RenderGraphLifetimeData data;
		data.renderGraphLifetimeResources = m_renderGraphLifetimeResources;
		for (size_t i = 0; i < m_stages.size(); ++i)
		{

			m_stages[i]->onRenderGraphCompiled(data);
		}
		 
		m_renderGraphLifetimeResources->allocate();
	}

	void RenderPipelineBase::shutdown()
	{
		for (size_t i = 0; i < m_stages.size(); ++i)
		{
			m_stages[i]->shutdown();
			delete m_stages[i];
		}
		m_stages.clear();

		m_renderResolutionDependantResources->deallocate();
		delete m_renderResolutionDependantResources;
		m_renderResolutionDependantResources = nullptr;

		m_renderGraphLifetimeResources->deallocate();
		delete m_renderGraphLifetimeResources;
		m_renderGraphLifetimeResources = nullptr;

		

		Gfx::destroyRenderGraph(m_gfxHandle, m_graph);

	}
	void RenderPipelineBase::prepare(const PrepareContext& cntx)
	{
		size_t renderResWidth = cntx.renderWidth;
		size_t renderResHeight = cntx.renderHeight;

		if (renderResWidth != m_renderResolutionWidth || renderResHeight != m_renderResolutionHeight || m_firstPrepareAfterInit)
		{
			if (!m_firstPrepareAfterInit)
			{
				m_renderResolutionDependantResources->deallocate();
			}


			m_renderResolutionWidth = renderResWidth;
			m_renderResolutionHeight = renderResHeight;
			RenderStage::RenderResolutionDependantResourcesData resolutionChangedData;
			resolutionChangedData.newRenderResolutionWidth = (uint32_t)m_renderResolutionWidth;
			resolutionChangedData.newRenderResolutionHeight = (uint32_t)m_renderResolutionHeight;
			resolutionChangedData.resolutionDependantResourcesPool = m_renderResolutionDependantResources;

			for (size_t i = 0; i < m_stages.size(); ++i)
			{
				m_stages[i]->onRenderResolutionChanged(resolutionChangedData);
			}

			m_renderResolutionDependantResources->allocate();
		}


		RenderStage::PrepareData stagePrepareContext;
		stagePrepareContext.swapChain = cntx.swapChain;
		for (size_t i = 0; i < m_stages.size(); ++i)
		{
			m_stages[i]->prepare(stagePrepareContext);
		}

		m_firstPrepareAfterInit = false;
	}

	void RenderPipelineBase::update(const UpdateContext& cntx)
	{
		RenderStage::UpdateData stageUpdateContext;

		//execute nodes (TODO: multithreaded)
		for (size_t i = 0; i < m_stages.size(); ++i)
		{
			m_stages[i]->update(stageUpdateContext);
		}
	}
	void RenderPipelineBase::execute()
	{
		m_graph->execute();
		
	}

	void RenderPipelineBase::addRenderStage(RenderStage* stage)
	{
		stage->setup(m_renderer, m_graph);
		stage->initialize();
		m_stages.push_back(stage);
	}
}