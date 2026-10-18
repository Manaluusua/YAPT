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
		m_pipelineLifetimeResources(nullptr),
		m_numberOfCommandBuffers(1),
		m_firstPrepareAfterInit(false),
		m_resolutionDependantResourcesNeedRebind(false)
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
		m_pipelineLifetimeResources = new RenderResourcesPool(m_renderer->getGfxHandle());

		createStages();
		compileRenderGraph();

		m_firstPrepareAfterInit = true;
	}

	void RenderPipelineBase::shutdown()
	{
		for (size_t i = 0; i < m_stages.size(); ++i)
		{
			m_stages[i]->shutdown();
			delete m_stages[i];
		}
		m_stages.clear();

		RenderResourcesPool** pools[] = { &m_renderResolutionDependantResources, &m_renderGraphLifetimeResources, &m_pipelineLifetimeResources };
		for (RenderResourcesPool** pool : pools)
		{
			(*pool)->deallocate();
			delete *pool;
			*pool = nullptr;
		}

		m_namedTextureMappings.clear();
		m_namedBufferMappings.clear();

		Gfx::destroyRenderGraph(m_gfxHandle, m_graph);
		m_graph = nullptr;
	}

	bool RenderPipelineBase::anyStageNeedsRecompile()
	{
		bool needsRecompile = false;
		for (size_t i = 0; i < m_stages.size(); ++i)
		{
			needsRecompile = m_stages[i]->needsRecompile() || needsRecompile;
		}
		return needsRecompile;
	}

	void RenderPipelineBase::compileRenderGraph()
	{
		m_graph->reset();

		for (size_t i = 0; i < m_stages.size(); ++i)
		{
			m_stages[i]->createNodes();
		}
		connectStages();

		m_graph->compile();
		m_graph->setupScheduling(m_numberOfCommandBuffers);

		m_namedTextureMappings.clear();
		m_namedBufferMappings.clear();
		afterRenderGraphCompiled();

		RenderStage::RenderGraphLifetimeData data;
		data.renderGraphLifetimeResources = m_renderGraphLifetimeResources;

		m_renderGraphLifetimeResources->beginGeneration();
		for (size_t i = 0; i < m_stages.size(); ++i)
		{
			m_stages[i]->onRenderGraphCompiled(data);
		}
		m_renderGraphLifetimeResources->endGeneration();

		m_resolutionDependantResourcesNeedRebind = true;
	}

	JobHandle RenderPipelineBase::prepare(const PrepareContext& cntx)
	{
		if (!m_firstPrepareAfterInit && anyStageNeedsRecompile())
		{
			compileRenderGraph();
		}

		size_t renderResWidth = cntx.renderWidth;
		size_t renderResHeight = cntx.renderHeight;

		bool resolutionChanged = renderResWidth != m_renderResolutionWidth || renderResHeight != m_renderResolutionHeight || m_firstPrepareAfterInit;
		if (resolutionChanged || m_resolutionDependantResourcesNeedRebind)
		{
			m_renderResolutionWidth = renderResWidth;
			m_renderResolutionHeight = renderResHeight;
			RenderStage::RenderResolutionDependantResourcesData resolutionChangedData;
			resolutionChangedData.newRenderResolutionWidth = (uint32_t)m_renderResolutionWidth;
			resolutionChangedData.newRenderResolutionHeight = (uint32_t)m_renderResolutionHeight;
			resolutionChangedData.resolutionDependantResourcesPool = m_renderResolutionDependantResources;
			resolutionChangedData.resolutionChanged = resolutionChanged;

			m_renderResolutionDependantResources->beginGeneration();
			for (size_t i = 0; i < m_stages.size(); ++i)
			{
				m_stages[i]->onRenderResolutionChanged(resolutionChangedData);
			}
			m_renderResolutionDependantResources->endGeneration();

			m_resolutionDependantResourcesNeedRebind = false;
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
	void RenderPipelineBase::prepareExecute(const ExecuteContext& cntx)
	{
		m_graph->prepareExecution(cntx.executeTasksPool);
	}

	void RenderPipelineBase::execute(const ExecuteContext& cntx, ExecuteOutput& output)
	{
		RenderStage::ExecuteData stageExecuteData;
		stageExecuteData.commandBuffersOut = &output.commandBuffers;
		for (size_t i = 0; i < m_stages.size(); ++i)
		{
			m_stages[i]->beforeExecute(stageExecuteData);
		}

		const CommandBufferHandle* buffers = nullptr;
		size_t bufferCount = 0;
		m_graph->recordCommandBuffers(buffers, bufferCount);
		output.commandBuffers.insert(output.commandBuffers.end(), buffers, buffers + bufferCount);
	}

	void RenderPipelineBase::afterSubmit()
	{
		m_graph->afterSubmit();
	}

	void RenderPipelineBase::addRenderStage(RenderStage* stage)
	{
		RenderStage::InitializeData data;
		data.pipelineLifetimeResources = m_pipelineLifetimeResources;

		stage->setup(m_renderer, m_graph);
		stage->initialize(data);
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