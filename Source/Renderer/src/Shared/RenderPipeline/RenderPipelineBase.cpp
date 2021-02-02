#include <Renderer/Shared/RenderPipeline/RenderPipelineBase.h>
#include <Renderer/Shared/RenderPipeline/RenderStage.h>
#include <Renderer/Shared/Utility/RenderResourcesPool.h>
#include <Renderer/Shared/CRenderer.h>
#include <algorithm>

namespace YAPT
{
	RenderPipelineBase::RenderPipelineBase()
		:m_gfxHandle(YAPT_NULL_HANDLE),
		m_cmdBufferPool(YAPT_NULL_HANDLE),
		m_graph(nullptr),
		m_renderer(nullptr),
		m_numberOfCmdBuffersPerFrame(0),
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

		Gfx::destroyCommandBufferPool(m_gfxHandle, m_cmdBufferPool);

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
		m_commandBuffersRecording.resize(m_numberOfCmdBuffersPerFrame);

		
		RenderGraphNode** nodes = m_graph->getNodes();
		size_t nodeCount = m_graph->getNodeCount();

		m_graph->beginExecution();

		//execute nodes (TODO: multithreaded)

		for (size_t cmdBufInd = 0; cmdBufInd < m_numberOfCmdBuffersPerFrame; ++cmdBufInd)
		{
			CommandBufferHandle buff = Gfx::startRecording(m_gfxHandle, m_cmdBufferPool, cmdBufInd);
			m_commandBuffersRecording[cmdBufInd] = buff;

			RenderGraphNodeExecutionContext context;
			context.cmdBuffer = buff;

			for (size_t nodeSequenceIndex = 0; nodeSequenceIndex < m_scheduledRenderGraphNodeGroups[cmdBufInd].renderNodeSequence.size(); ++nodeSequenceIndex)
			{
				const RenderNodeSequence& sequence = m_scheduledRenderGraphNodeGroups[cmdBufInd].renderNodeSequence[nodeSequenceIndex];
				m_graph->executeNodes(nodes + sequence.offset, sequence.count, context);
				
			}

			Gfx::stopRecording(m_gfxHandle, buff);
		}

		m_graph->endExecution();

		Gfx::submitCommandBuffers(m_gfxHandle, m_commandBuffersRecording.data(), m_commandBuffersRecording.size());

		m_graph->afterRenderGraphSubmit();

		m_commandBuffersRecording.clear();
		
	}

	void RenderPipelineBase::addRenderStage(RenderStage* stage)
	{
		

		stage->setup(m_renderer, m_graph);
		stage->initialize();
		m_stages.push_back(stage);
	}

	void RenderPipelineBase::setupCommandBufferPool(size_t numberOfBuffersPerFrame)
	{
		m_cmdBufferPool = Gfx::createCommandBufferPool(m_gfxHandle, numberOfBuffersPerFrame, 0, nullptr);
		m_numberOfCmdBuffersPerFrame = numberOfBuffersPerFrame;
	}

	void RenderPipelineBase::setupRenderGraphSchedulingGroups(size_t maximumNumberOfGroups)
	{
		size_t nodeCount = m_graph->getNodeCount();
		size_t groupCount = maximumNumberOfGroups > nodeCount ? nodeCount : maximumNumberOfGroups;

		assert(nodeCount > 0);

		size_t nodesPerGroup = nodeCount / groupCount;
		size_t extraNodes = nodeCount % groupCount;

		size_t groupsPerCmdBuffer = groupCount / m_numberOfCmdBuffersPerFrame;
		size_t extraGroups = groupCount % m_numberOfCmdBuffersPerFrame;

		size_t cmdBuffersToUse = std::min(m_numberOfCmdBuffersPerFrame, groupCount);


		m_scheduledRenderGraphNodeGroups.resize(cmdBuffersToUse);

		size_t currentNodeIndex = 0;

		for (size_t bufferIndex = 0; bufferIndex < cmdBuffersToUse; ++bufferIndex)
		{

			ScheduledRenderNodesPerBuffer& perBufferInfo = m_scheduledRenderGraphNodeGroups[bufferIndex];

			size_t currentGroupCount = groupsPerCmdBuffer;
			if (extraGroups > 0)
			{
				currentGroupCount += 1;
				--extraGroups;
			}

			perBufferInfo.renderNodeSequence.resize(currentGroupCount);

			for (size_t groupIndex = 0; groupIndex < currentGroupCount; ++groupIndex)
			{
				size_t currentNodeCount = nodesPerGroup;
				if (extraNodes > 0)
				{
					++currentNodeCount;
					--extraNodes;
				}

				RenderNodeSequence& seq = perBufferInfo.renderNodeSequence[groupIndex];
				seq.offset = currentNodeIndex;
				seq.count = currentNodeCount;
				currentNodeIndex += currentNodeCount;
			}

		}

	}

	
}