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
	JobHandle RenderPipelineBase::prepare(const PrepareContext& cntx)
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
		}
		m_jobsToWait.clear();
		m_jobsToWait.reserve(m_stages.size());
		RenderStage::PrepareData stagePrepareContext;
		stagePrepareContext.swapChain = cntx.swapChain;
		stagePrepareContext.prepareTasksPool = cntx.prepareTasksPool;
		for (size_t i = 0; i < m_stages.size(); ++i)
		{
			JobHandle h = m_stages[i]->prepare(stagePrepareContext);
			if (h.isValid())
			{
				m_jobsToWait.push_back(h);
			}
		}

		m_firstPrepareAfterInit = false;
		if (m_jobsToWait.size() > 0)
		{
			return cntx.prepareTasksPool->combineDependencies(m_jobsToWait.data(), m_jobsToWait.size());
		}
		else
		{
			return {};
		}
	}

	JobHandle RenderPipelineBase::update(const UpdateContext& cntx)
	{
		RenderStage::UpdateData stageUpdateContext;
		stageUpdateContext.updateTasksPool = cntx.updateTasksPool;

		m_jobsToWait.clear();
		m_jobsToWait.reserve(m_stages.size());

		//execute nodes (TODO: multithreaded)
		for (size_t i = 0; i < m_stages.size(); ++i)
		{
			JobHandle h = m_stages[i]->update(stageUpdateContext);
			if (h.isValid())
			{
				m_jobsToWait.push_back(h);
			}
		}

		m_firstPrepareAfterInit = false;
		if (m_jobsToWait.size() > 0)
		{
			return cntx.updateTasksPool->combineDependencies(m_jobsToWait.data(), m_jobsToWait.size());
		}
		else
		{
			return {};
		}
	}
	void RenderPipelineBase::execute(const ExecuteContext& cntx)
	{
		for (size_t i = 0; i < m_stages.size(); ++i)
		{
			m_stages[i]->beforeExecute();
		}

		m_graph->execute(cntx.executeTasksPool);
		
	}

	void RenderPipelineBase::addRenderStage(RenderStage* stage)
	{
		stage->setup(m_renderer, m_graph);
		stage->initialize();
		m_stages.push_back(stage);
	}

	void RenderPipelineBase::setNamedResourceMapping(NamedRenderPipelineTexture tex, size_t nodeIndex, size_t slotIndex)
	{
		m_namedTextureMappings.push_back({ tex, nodeIndex, slotIndex });
	}
	void RenderPipelineBase::setNamedResourceMapping(NamedRenderPipelineBuffer buff, size_t nodeIndex, size_t slotIndex)
	{
		m_namedBufferMappings.push_back({ buff, nodeIndex, slotIndex });
	}

	TextureHandle RenderPipelineBase::getNamedResource(NamedRenderPipelineTexture tex)
	{
		for (size_t i = 0; i < m_namedTextureMappings.size(); ++i)
		{
			const NamedRenderPipelineTextureMapping& mapping = m_namedTextureMappings[i];
			if (mapping.namedTexture == tex)
			{
				RenderGraphResourceId resId = getRenderGraph()->getRenderGraphResourceIdUsedInSlot(mapping.nodeIndex, mapping.slotIndex);
				return getRenderGraph()->getTextureForResourceId(resId);
			}
		}

		return YAPT_NULL_HANDLE;
	}
	BufferHandle RenderPipelineBase::getNamedResource(NamedRenderPipelineBuffer buff)
	{
		for (size_t i = 0; i < m_namedBufferMappings.size(); ++i)
		{
			const NamedRenderPipelineBufferMapping& mapping = m_namedBufferMappings[i];
			if (mapping.namedBuffer == buff)
			{
				RenderGraphResourceId resId = getRenderGraph()->getRenderGraphResourceIdUsedInSlot(mapping.nodeIndex, mapping.slotIndex);
				return getRenderGraph()->getBufferForResourceId(resId);
			}
		}

		return YAPT_NULL_HANDLE;
	}
}