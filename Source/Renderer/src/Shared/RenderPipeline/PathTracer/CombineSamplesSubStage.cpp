#include <Renderer/Shared/RenderPipeline/PathTracer/CombineSamplesSubStage.h>
#include <Gfx/RenderGraph/RenderGraph.h>
#include <Renderer/Shared/CRenderer.h>
#include <Renderer/Shared/Utility/CoreRenderResourcesUtility.h>

#define MERGE_SAMPLES_WG_SIZE 8

namespace YAPT
{
	CombineSamplesSubStage::CombineSamplesSubStage()
	{

	}
	CombineSamplesSubStage::~CombineSamplesSubStage()
	{

	}

	void CombineSamplesSubStage::initialize(CRenderer* rend, RenderGraph* graph)
	{
		m_graph = graph;
		m_renderer = rend;

		RenderGraphNodeSlotDefinition slotdefsMergeNode[] =
		{
		{{ResourceDimension::TEXTURE_2D,
			ResourceFormat::RGBA32_SFLOAT,
			RESOURCE_USAGE_STORAGE_TEXTURE,
			ACCESS_FLAGS_WRITE,
			SHADERSTAGE_COMPUTE,
			1,
			1
		}},
		{{ResourceDimension::TEXTURE_2D,
			ResourceFormat::RGBA16_SFLOAT,
			RESOURCE_USAGE_SAMPLED_TEXTURE,
			ACCESS_FLAGS_READ,
			SHADERSTAGE_FRAGMENT,
			1,
			1
		}}
		};

		m_mergeNode = m_graph->createComputeNode(2, slotdefsMergeNode, []
		(RenderGraphNode* node, const RenderGraphNodeExecutionContext& execContext, void* usrData)
			{
				static_cast<CombineSamplesSubStage*>(usrData)->executeMergeToPrevious(execContext);
			},
			this, "MergeRtResultsNode");
	}

	void CombineSamplesSubStage::setInput(RenderGraphNode* node, size_t slot)
	{
		m_graph->createEdge(node, slot, m_mergeNode, 1);
	}
	void CombineSamplesSubStage::getOutput(RenderGraphNode** node, size_t& slotOut)
	{
		*node = m_mergeNode;
		slotOut = 0;
	}

	void CombineSamplesSubStage::shutdown()
	{
		m_clearMergeBufferPass.deinit();
		m_mergePass.deinit();
	}
	void CombineSamplesSubStage::onRenderGraphCompiled(const RenderStage::RenderGraphLifetimeData& data)
	{
		//mergepass init (properly setup later)
		ShaderLoader* loader = m_renderer->getShaderLoader();
		const ShaderLoader::ShaderPipelineInfo* merge = loader->getShaderPipeline("mergeRaytraceResults");

		StaticSamplerEntry samplers[] = { {"colorSampler", m_renderer->getCoreResources()->getDefaultSampler(DefaultSamplerType::LINEAR_REPEAT)} };
		m_mergePass.init(m_renderer, merge, samplers, countOf(samplers));
		m_mergeSamplesConstants.init(data.renderGraphLifetimeResources);

		const ShaderLoader::ShaderPipelineInfo* clearMergeBuffer = loader->getShaderPipeline("clearRaytraceMergeTarget");
		m_clearMergeBufferPass.init(m_renderer, clearMergeBuffer, nullptr, 0);
		m_clearMergeBufferConstants.init(data.renderGraphLifetimeResources);
	}
	void CombineSamplesSubStage::onRenderResolutionChanged(const RenderStage::RenderResolutionDependantResourcesData& data, uvec2 newResolution)
	{

		m_renderResolution = newResolution;

		m_mergePass.createPipelineState();
		m_clearMergeBufferPass.createPipelineState();
		
		RenderGraphResourceId resId = m_mergeNode->getRenderGraphResourceIdForSlot(0);
		const RenderGraphResourceDescription& desc = m_graph->getRenderGraphResourceDescription(resId);
		TextureDesc textureDesc(desc.resourceDimensions, desc.resourceFormat, desc.resourceUsage, m_renderResolution.x, m_renderResolution.y, 1, 1);
		TextureHandle tex = data.resolutionDependantResourcesPool->requestTexture(textureDesc, "merged RT result");
		m_graph->setRenderGraphResourceTexture(resId, tex);
		
	}

	void CombineSamplesSubStage::update(const UpdateParams& params)
	{
		bool haveTexturesChanged = m_graph->isResourceBoundThisFrame(m_mergeNode->getRenderGraphResourceIdForSlot(0));
		haveTexturesChanged = haveTexturesChanged || m_graph->isResourceBoundThisFrame(m_mergeNode->getRenderGraphResourceIdForSlot(1));
		if (haveTexturesChanged)
		{
			//clear accumulation buffer
			{
				TextureViewHandle mergeTarget = m_graph->getTextureViewFromNodeSlot(m_mergeNode->getSortedIndex(), 0);

				DescriptorSetUpdate updates[] = {
					{0, 0, 1, DescriptorPtr(m_clearMergeBufferConstants.getViewPtr())},
					{1, 0, 1, DescriptorPtr(&mergeTarget)},
				};
				m_clearMergeBufferPass.reserveAndUpdateDescriptorSet(0, updates, countOf(updates));
			}

			//Merge
			{
				TextureViewHandle mergeTarget = m_graph->getTextureViewFromNodeSlot(m_mergeNode->getSortedIndex(), 0);
				TextureViewHandle mergeSource = m_graph->getTextureViewFromNodeSlot(m_mergeNode->getSortedIndex(), 1);

				DescriptorSetUpdate updates[] = {
					{0, 0, 1, DescriptorPtr(m_mergeSamplesConstants.getViewPtr())},
					{1, 0, 1, DescriptorPtr(&mergeSource)},
					{2, 0, 1, DescriptorPtr(&mergeTarget)}
				};
				m_mergePass.reserveAndUpdateDescriptorSet(0, updates, countOf(updates));
			}

		}
		//update constants
		{
			float targetPixelWidth = 1.f / m_renderResolution.x;
			float targetPixelHeight = 1.f / m_renderResolution.y;


			if (params.clearAccumulated)
			{
				ClearAccumulatedSamplesParams* params = m_clearMergeBufferConstants.getData();
				params->targetTextureDimensions = glm::uvec4(m_renderResolution.x, m_renderResolution.y, 0, 0);
				params->clearValue = vec4p(0.0, 0.f, 0.f, 0.f);
				m_clearMergeBufferConstants.flush();
			}

			MergeNewSamplesParams* mergeSamplesParams = m_mergeSamplesConstants.getData();
			mergeSamplesParams->sourceTextureDimensions = glm::uvec2(params.sourceTextureResolution.x, params.sourceTextureResolution.y);
			mergeSamplesParams->targetTextureOffsetScaleBias = params.targetOffsetScaleBias;
			mergeSamplesParams->sampleCount = params.samplesPerPixel;

			m_mergeSamplesConstants.flush();
		}
		m_lastUpdateParams = params;
	}


	void CombineSamplesSubStage::executeMergeToPrevious(const RenderGraphNodeExecutionContext& exec)
	{
		if (m_lastUpdateParams.clearAccumulated)
		{
			uint32_t dispatchX = (m_renderResolution.x + MERGE_SAMPLES_WG_SIZE - 1) / MERGE_SAMPLES_WG_SIZE;
			uint32_t dispatchY = (m_renderResolution.y + MERGE_SAMPLES_WG_SIZE - 1) / MERGE_SAMPLES_WG_SIZE;

			m_clearMergeBufferPass.dispatch(exec.cmdBuffer, dispatchX, dispatchY, 1);
		}


		uint32_t rtWidth = m_lastUpdateParams.sourceTextureResolution.x;
		uint32_t rtHeight = m_lastUpdateParams.sourceTextureResolution.y;

		uint32_t dispatchX = (rtWidth + MERGE_SAMPLES_WG_SIZE - 1) / MERGE_SAMPLES_WG_SIZE;
		uint32_t dispatchY = (rtHeight + MERGE_SAMPLES_WG_SIZE - 1) / MERGE_SAMPLES_WG_SIZE;

		m_mergePass.dispatch(exec.cmdBuffer, dispatchX, dispatchY, 1);
	}
}