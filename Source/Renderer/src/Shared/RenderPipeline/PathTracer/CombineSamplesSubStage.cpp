#include <Renderer/Shared/RenderPipeline/PathTracer/CombineSamplesSubStage.h>
#include <Gfx/RenderGraph/RenderGraph.h>
#include <Renderer/Shared/CRenderer.h>
#include <Renderer/Shared/Utility/CoreRenderResourcesUtility.h>
#include <Renderer/Shared/RenderPipeline/PathTracer/PathIntegratorSubStage.h>
#include <Math/RandUtility.h>
#define MERGE_SAMPLES_WG_SIZE 8
#define DENOISE_WG_SIZE 8

namespace YAPT
{
	CombineSamplesSubStage::CombineSamplesSubStage()
		:m_precalculatedDenoiseKernelWeightsBuffer(RESOURCE_USAGE_COPY_DESTINATION | RESOURCE_USAGE_STORAGE_BUFFER, true),
		m_lastKernelSize(0),
		m_lastKernelTau(0)
	{

	}
	CombineSamplesSubStage::~CombineSamplesSubStage()
	{

	}

	void CombineSamplesSubStage::initialize(CRenderer* rend, RenderGraph* graph)
	{
		m_graph = graph;
		m_renderer = rend;
		

		{
			RenderGraphNodeSlotDefinition slotdefsClearNode[] =
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
				ResourceFormat::R32_UINT,
				RESOURCE_USAGE_STORAGE_TEXTURE,
				ACCESS_FLAGS_WRITE,
				SHADERSTAGE_COMPUTE,
				1,
				1
			}}
			};

			m_clearNode = m_graph->createComputeNode(countOf(slotdefsClearNode), slotdefsClearNode, []
			(RenderGraphNode* node, const RenderGraphNodeExecutionContext& execContext, void* usrData)
				{
					static_cast<CombineSamplesSubStage*>(usrData)->executeClear(execContext);
				},
				this, "ClearRtResultsNode");
		}

		{
			RenderGraphNodeSlotDefinition slotdefsMergeNode[] =
			{
			{{ResourceDimension::TEXTURE_2D,
				ResourceFormat::UNKNOWN,
				RESOURCE_USAGE_STORAGE_TEXTURE,
				ACCESS_FLAGS_READ_WRITE,
				SHADERSTAGE_COMPUTE,
				1,
				1
			}},
			{{ResourceDimension::TEXTURE_2D,
				ResourceFormat::UNKNOWN,
				RESOURCE_USAGE_STORAGE_TEXTURE,
				ACCESS_FLAGS_READ_WRITE,
				SHADERSTAGE_COMPUTE,
				1,
				1
			}},
			{{ResourceDimension::TEXTURE_2D,
				PathIntegratorSubStage::SampleImageFormat,
				RESOURCE_USAGE_SAMPLED_TEXTURE,
				ACCESS_FLAGS_READ,
				SHADERSTAGE_COMPUTE,
				1,
				1
			}}
			};

			m_mergeNode = m_graph->createComputeNode(countOf(slotdefsMergeNode), slotdefsMergeNode, []
			(RenderGraphNode* node, const RenderGraphNodeExecutionContext& execContext, void* usrData)
				{
					static_cast<CombineSamplesSubStage*>(usrData)->executeMergeToPrevious(execContext);
				},
				this, "MergeRtResultsNode");
		}

		{
			RenderGraphNodeSlotDefinition slotdefsDenoiseNode[] =
			{
			{{
				ResourceDimension::TEXTURE_2D,
				ResourceFormat::RGBA32_SFLOAT,
				RESOURCE_USAGE_STORAGE_TEXTURE,
				ACCESS_FLAGS_WRITE,
				SHADERSTAGE_COMPUTE,
				1,
				1
			}},
			{{
				ResourceDimension::TEXTURE_2D,
				ResourceFormat::UNKNOWN,
				RESOURCE_USAGE_SAMPLED_TEXTURE,
				ACCESS_FLAGS_READ,
				SHADERSTAGE_COMPUTE,
				1,
				1
			}},
			{{
				ResourceDimension::TEXTURE_2D,
				ResourceFormat::UNKNOWN,
				RESOURCE_USAGE_SAMPLED_TEXTURE,
				ACCESS_FLAGS_READ,
				SHADERSTAGE_COMPUTE,
				1,
				1
			}},
			{{
				ResourceDimension::TEXTURE_2D,
				ResourceFormat::UNKNOWN,
				RESOURCE_USAGE_SAMPLED_TEXTURE,
				ACCESS_FLAGS_READ,
				SHADERSTAGE_COMPUTE,
				1,
				1
			}}
			};

			m_denoiseNode = m_graph->createComputeNode(countOf(slotdefsDenoiseNode), slotdefsDenoiseNode, []
			(RenderGraphNode* node, const RenderGraphNodeExecutionContext& execContext, void* usrData)
				{
					static_cast<CombineSamplesSubStage*>(usrData)->executeDenoise(execContext);
				},
				this, "DenoiseNode");
		}
		m_graph->createEdge(m_clearNode, 0, m_mergeNode, 0);
		m_graph->createEdge(m_clearNode, 1, m_mergeNode, 1);
		m_graph->createEdge(m_mergeNode, 0, m_denoiseNode, 1);

	}

	void CombineSamplesSubStage::setInput(InputResource resource, RenderGraphNode* node, size_t slot)
	{
		switch (resource)
		{
		case InputResource::COLOR:
			m_graph->createEdge(node, slot, m_mergeNode, 2);
			break;
		case InputResource::MATERIAL_PARAMS0:
			m_graph->createEdge(node, slot, m_denoiseNode, 2);
			break;
		case InputResource::MATERIAL_PARAMS1:
			m_graph->createEdge(node, slot, m_denoiseNode, 3);
			break;
		default:
			assert(!"unknown input resource");
		}
	}
	void CombineSamplesSubStage::getOutput(OutputResource outputResource, RenderGraphNode** node, size_t& slotOut)
	{
		switch (outputResource)
		{
		case OutputResource::COLOR:
			*node = m_denoiseNode;
			slotOut = 0;
			break;
		default:
			assert(!"unknown output resource");
		}
	}

	void CombineSamplesSubStage::shutdown()
	{
		m_clearMergeBufferPass.deinit();
		m_mergePass.deinit();
		m_denoisePass.deinit();
	}
	void CombineSamplesSubStage::onRenderGraphCompiled(const RenderStage::RenderGraphLifetimeData& data)
	{
		//mergepass init (properly setup later)
		ShaderLoader* loader = m_renderer->getShaderLoader();
		{
			const ShaderLoader::ShaderPipelineInfo* merge = loader->getShaderPipeline("mergeRaytraceResults");
			StaticSamplerEntry samplers[] = { {"colorSampler", m_renderer->getCoreResources()->getDefaultSampler(DefaultSamplerType::LINEAR_REPEAT)} };
			m_mergePass.init(m_renderer, merge, samplers, countOf(samplers));
			m_mergeSamplesConstants.init(data.renderGraphLifetimeResources);
		}
		
		{
			const ShaderLoader::ShaderPipelineInfo* clearMergeBuffer = loader->getShaderPipeline("clearRaytraceMergeTarget");
			m_clearMergeBufferPass.init(m_renderer, clearMergeBuffer, nullptr, 0);
			m_clearMergeBufferConstants.init(data.renderGraphLifetimeResources);
		}

		{
			const ShaderLoader::ShaderPipelineInfo* denoise = loader->getShaderPipeline("denoise");
			m_denoisePass.init(m_renderer, denoise, nullptr, 0);
			m_denoiseConstants.init(data.renderGraphLifetimeResources);
			m_precalculatedDenoiseKernelWeightsBuffer.init(m_renderer->getGfxHandle());
		}
	}
	void CombineSamplesSubStage::onRenderResolutionChanged(const RenderStage::RenderResolutionDependantResourcesData& data, uvec2 newResolution)
	{

		m_renderResolution = newResolution;

		m_mergePass.createPipelineState();
		m_clearMergeBufferPass.createPipelineState();
		m_denoisePass.createPipelineState();
		
		{
			RenderGraphResourceId resId = m_mergeNode->getRenderGraphResourceIdForSlot(0);
			const RenderGraphResourceDescription& desc = m_graph->getRenderGraphResourceDescription(resId);
			TextureDesc textureDesc(desc.resourceDimensions, desc.resourceFormat, desc.resourceUsage, m_renderResolution.x, m_renderResolution.y, 1, 1);
			TextureHandle tex = data.resolutionDependantResourcesPool->requestTexture(textureDesc, "merged RT result");
			m_graph->setRenderGraphResourceTexture(resId, tex);
		}

		{
			RenderGraphResourceId resId = m_mergeNode->getRenderGraphResourceIdForSlot(1);
			const RenderGraphResourceDescription& desc = m_graph->getRenderGraphResourceDescription(resId);
			TextureDesc textureDesc(desc.resourceDimensions, desc.resourceFormat, desc.resourceUsage, m_renderResolution.x, m_renderResolution.y, 1, 1);
			TextureHandle tex = data.resolutionDependantResourcesPool->requestTexture(textureDesc, "Sample Count");
			m_graph->setRenderGraphResourceTexture(resId, tex);
		}

		{
			RenderGraphResourceId resId = m_denoiseNode->getRenderGraphResourceIdForSlot(0);
			const RenderGraphResourceDescription& desc = m_graph->getRenderGraphResourceDescription(resId);
			TextureDesc textureDesc(desc.resourceDimensions, desc.resourceFormat, desc.resourceUsage, m_renderResolution.x, m_renderResolution.y, 1, 1);
			TextureHandle tex = data.resolutionDependantResourcesPool->requestTexture(textureDesc, "denoised RT result");
			m_graph->setRenderGraphResourceTexture(resId, tex);
		}
		
		
	}

	void CombineSamplesSubStage::update(const UpdateParams& params)
	{
		bool haveTexturesChanged = m_graph->isResourceBoundThisFrame(m_mergeNode->getRenderGraphResourceIdForSlot(0));
		haveTexturesChanged = haveTexturesChanged || m_graph->isResourceBoundThisFrame(m_mergeNode->getRenderGraphResourceIdForSlot(1));
		haveTexturesChanged = haveTexturesChanged || m_graph->isResourceBoundThisFrame(m_denoiseNode->getRenderGraphResourceIdForSlot(1));
		haveTexturesChanged = haveTexturesChanged || m_graph->isResourceBoundThisFrame(m_denoiseNode->getRenderGraphResourceIdForSlot(2));
		haveTexturesChanged = haveTexturesChanged || m_graph->isResourceBoundThisFrame(m_denoiseNode->getRenderGraphResourceIdForSlot(3));

		CRendererConfiguration& config = m_renderer->getConcreteRendererConfiguration();
		int32_t denoiseMode = config.getRendererVarValueInternal<int32_t>(RVARNAME_DENOISE_MODE);
		int32_t kernelWidth = config.getRendererVarValueInternal<int32_t>(RVARNAME_DENOISE_WIDTH);
		float tau = config.getRendererVarValueInternal<float>(RVARNAME_DENOISE_TAU);

		if(kernelWidth != m_lastKernelSize || m_lastKernelTau != tau)
		{
			if (m_precalculatedDenoiseKernelWeightsBuffer.getAllocatedEntryCount() < kernelWidth)
			{
				m_precalculatedDenoiseKernelWeightsBuffer.allocate(kernelWidth, "precalculatedKernelWeights");
			}
			char* data = m_precalculatedDenoiseKernelWeightsBuffer.map(0, kernelWidth);
			precalculateKernelWeights(kernelWidth, tau, reinterpret_cast<float*>(data));
			m_lastKernelSize = kernelWidth;
			m_lastKernelTau = tau;
		}

		if (haveTexturesChanged)
		{
			//clear accumulation buffer
			{
				TextureViewHandle mergeTarget = m_graph->getTextureViewFromNodeSlot(m_mergeNode->getSortedIndex(), 0);
				TextureViewHandle sampleCountTarget = m_graph->getTextureViewFromNodeSlot(m_mergeNode->getSortedIndex(), 1);

				DescriptorSetUpdate updates[] = {
					{0, 0, 1, DescriptorPtr(m_clearMergeBufferConstants.getViewPtr())},
					{1, 0, 1, DescriptorPtr(&mergeTarget)},
					{2, 0, 1, DescriptorPtr(&sampleCountTarget)},
				};
				m_clearMergeBufferPass.reserveAndUpdateDescriptorSet(0, updates, countOf(updates));
			}

			//Merge
			{
				TextureViewHandle mergeTarget = m_graph->getTextureViewFromNodeSlot(m_mergeNode->getSortedIndex(), 0);
				TextureViewHandle sampleCountTarget = m_graph->getTextureViewFromNodeSlot(m_mergeNode->getSortedIndex(), 1);
				TextureViewHandle mergeSource = m_graph->getTextureViewFromNodeSlot(m_mergeNode->getSortedIndex(), 2);

				DescriptorSetUpdate updates[] = {
					{0, 0, 1, DescriptorPtr(m_mergeSamplesConstants.getViewPtr())},
					{1, 0, 1, DescriptorPtr(&mergeSource)},
					{2, 0, 1, DescriptorPtr(&mergeTarget)},
					{3, 0, 1, DescriptorPtr(&sampleCountTarget)}
				};
				m_mergePass.reserveAndUpdateDescriptorSet(0, updates, countOf(updates));
			}

			//denoise
			{
				TextureViewHandle denoiseTarget = m_graph->getTextureViewFromNodeSlot(m_denoiseNode->getSortedIndex(), 0);
				TextureViewHandle denoiseSource = m_graph->getTextureViewFromNodeSlot(m_denoiseNode->getSortedIndex(), 1);
				TextureViewHandle matParams0 = m_graph->getTextureViewFromNodeSlot(m_denoiseNode->getSortedIndex(), 2);
				TextureViewHandle matParams1 = m_graph->getTextureViewFromNodeSlot(m_denoiseNode->getSortedIndex(), 3);

				BufferViewHandle precalculatedKernelWeights = m_precalculatedDenoiseKernelWeightsBuffer.getBufferViewHandle();

				DescriptorSetUpdate updates[] = {
					{0, 0, 1, DescriptorPtr(m_denoiseConstants.getViewPtr())},
					{1, 0, 1, DescriptorPtr(&precalculatedKernelWeights)},
					{2, 0, 1, DescriptorPtr(&denoiseSource)},
					{3, 0, 1, DescriptorPtr(&matParams0)},
					{4, 0, 1, DescriptorPtr(&matParams1)},
					{5, 0, 1, DescriptorPtr(&denoiseTarget) }
				};
				m_denoisePass.reserveAndUpdateDescriptorSet(0, updates, countOf(updates));
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

			{
				MergeNewSamplesParams* mergeSamplesParams = m_mergeSamplesConstants.getData();
				mergeSamplesParams->sourceTextureDimensions = glm::uvec2(params.sourceTextureResolution.x, params.sourceTextureResolution.y);
				mergeSamplesParams->targetTextureOffsetScaleBias = params.targetOffsetScaleBias;
				mergeSamplesParams->targetTextureDimensions = params.targetTextureResolution;
				mergeSamplesParams->sampleCount = params.samplesPerPixel;
				MathUtils::generateSobolSequence(1u, 4u, glm::value_ptr(mergeSamplesParams->randomSequence), (uint32_t)params.samplesPerPixel);
				m_mergeSamplesConstants.flush();
			}

			{
				DenoiseParams* denoiseParams = m_denoiseConstants.getData();
				denoiseParams->denoiseMode = uvec4p(denoiseMode, kernelWidth, 0, 0);
				denoiseParams->textureDimensions = vec4p(m_renderResolution.x, m_renderResolution.y, targetPixelWidth, targetPixelHeight);
				m_denoiseConstants.flush();
			}
		}
		m_lastUpdateParams = params;
	}

	void CombineSamplesSubStage::executeClear(const RenderGraphNodeExecutionContext& exec)
	{
		if (m_lastUpdateParams.clearAccumulated)
		{
			uint32_t dispatchX = (m_renderResolution.x + MERGE_SAMPLES_WG_SIZE - 1) / MERGE_SAMPLES_WG_SIZE;
			uint32_t dispatchY = (m_renderResolution.y + MERGE_SAMPLES_WG_SIZE - 1) / MERGE_SAMPLES_WG_SIZE;

			m_clearMergeBufferPass.dispatch(exec.cmdBuffer, dispatchX, dispatchY, 1);
		}
	}

	



	void CombineSamplesSubStage::executeMergeToPrevious(const RenderGraphNodeExecutionContext& exec)
	{
		uint32_t rtWidth = m_lastUpdateParams.targetTextureResolution.x;
		uint32_t rtHeight = m_lastUpdateParams.targetTextureResolution.y;

		uint32_t dispatchX = (rtWidth + MERGE_SAMPLES_WG_SIZE - 1) / MERGE_SAMPLES_WG_SIZE;
		uint32_t dispatchY = (rtHeight + MERGE_SAMPLES_WG_SIZE - 1) / MERGE_SAMPLES_WG_SIZE;

		m_mergePass.dispatch(exec.cmdBuffer, dispatchX, dispatchY, 1);
	}


	void CombineSamplesSubStage::executeDenoise(const RenderGraphNodeExecutionContext& exec)
	{

		uint32_t rtWidth = m_lastUpdateParams.targetTextureResolution.x;
		uint32_t rtHeight = m_lastUpdateParams.targetTextureResolution.y;

		uint32_t dispatchX = (rtWidth + DENOISE_WG_SIZE - 1) / DENOISE_WG_SIZE;
		uint32_t dispatchY = (rtHeight + DENOISE_WG_SIZE - 1) / MERGE_SAMPLES_WG_SIZE;

		m_denoisePass.dispatch(exec.cmdBuffer, dispatchX, dispatchY, 1);
	}

	float sinc(float x)
	{
		if (x == 0)
			return 1;
		x *= glm::pi<float>();
		return std::sin(x) / x;
	}

	void CombineSamplesSubStage::precalculateKernelWeights(uint32_t width, float tau, float* dataPtr)
	{
		if (width == 0) return;
		dataPtr[0] = 1;

		for (uint32_t i = 1; i < width; ++i)
		{
			float w = sinc((float)i) * sinc((float)i / tau);
			dataPtr[i] = w;
		}
	}
}