#include <Renderer/Shared/RenderPipeline/PathTracer/CombineSamplesSubStage.h>
#include <Gfx/RenderGraph/RenderGraph.h>
#include <Renderer/Shared/CRenderer.h>
#include <Renderer/Shared/Utility/CoreRenderResourcesUtility.h>
#include <Renderer/Shared/RenderPipeline/PathTracer/PathIntegratorSubStage.h>
#include <Renderer/Shared/Utility/RenderAPIAbstractionUtility.h>
#include <Math/RandUtility.h>
#define MERGE_SAMPLES_WG_SIZE 8
#define DENOISE_WG_SIZE 8
#define ACCUMULATION_TARGET_COUNT 2
#define ACCUMULATION_TARGET_FORMAT (ResourceFormat::RGBA32_SFLOAT)


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
		

		{
			RenderGraphNodeSlotDefinition slotdefsClearNode[] =
			{
			{{ResourceDimension::TEXTURE_2D_ARRAY,
				ACCUMULATION_TARGET_FORMAT,
				RESOURCE_USAGE_STORAGE_TEXTURE,
				ACCESS_FLAGS_WRITE,
				SHADERSTAGE_COMPUTE,
				1,
				ACCUMULATION_TARGET_COUNT
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
				ACCUMULATION_TARGET_FORMAT,
				RESOURCE_USAGE_STORAGE_TEXTURE,
				ACCESS_FLAGS_READ_WRITE,
				SHADERSTAGE_COMPUTE,
				1,
				1
			}},
			{{ResourceDimension::TEXTURE_2D_ARRAY,
				ACCUMULATION_TARGET_FORMAT,
				RESOURCE_USAGE_STORAGE_TEXTURE,
				ACCESS_FLAGS_READ_WRITE,
				SHADERSTAGE_COMPUTE,
				1,
				ACCUMULATION_TARGET_COUNT
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
				ResourceFormat::R32_SFLOAT,
				RESOURCE_USAGE_STORAGE_TEXTURE,
				ACCESS_FLAGS_WRITE,
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
			{{ //denoise color out
				ResourceDimension::TEXTURE_2D,
				ResourceFormat::RGBA32_SFLOAT,
				RESOURCE_USAGE_STORAGE_TEXTURE,
				ACCESS_FLAGS_WRITE,
				SHADERSTAGE_COMPUTE,
				1,
				1
			}},
				{{ //denoise var out
				ResourceDimension::TEXTURE_2D,
				ResourceFormat::R32_SFLOAT,
				RESOURCE_USAGE_STORAGE_TEXTURE,
				ACCESS_FLAGS_WRITE,
				SHADERSTAGE_COMPUTE,
				1,
				1
			}},
			{{ //denoise color in
				ResourceDimension::TEXTURE_2D,
				ResourceFormat::UNKNOWN,
				RESOURCE_USAGE_SAMPLED_TEXTURE,
				ACCESS_FLAGS_READ,
				SHADERSTAGE_COMPUTE,
				1,
				1
			}},
			{{ //denoise var in
				ResourceDimension::TEXTURE_2D,
				ResourceFormat::UNKNOWN,
				RESOURCE_USAGE_SAMPLED_TEXTURE,
				ACCESS_FLAGS_READ,
				SHADERSTAGE_COMPUTE,
				1,
				1
			}},
			{{ //MATERIAL_PARAMS0
				ResourceDimension::TEXTURE_2D,
				ResourceFormat::UNKNOWN,
				RESOURCE_USAGE_SAMPLED_TEXTURE,
				ACCESS_FLAGS_READ,
				SHADERSTAGE_COMPUTE,
				1,
				1
			}},
			{{ //MATERIAL_PARAMS1
				ResourceDimension::TEXTURE_2D,
				ResourceFormat::UNKNOWN,
				RESOURCE_USAGE_SAMPLED_TEXTURE,
				ACCESS_FLAGS_READ,
				SHADERSTAGE_COMPUTE,
				1,
				1
			}},
			{{ //single sample color (debug)
				ResourceDimension::TEXTURE_2D,
				ResourceFormat::UNKNOWN,
				RESOURCE_USAGE_SAMPLED_TEXTURE,
				ACCESS_FLAGS_READ,
				SHADERSTAGE_COMPUTE,
				1,
				1
			}}
			};

			static constexpr int DENOISE_PASSES_COUNT = 5;
			m_denoisePasses.resize(DENOISE_PASSES_COUNT);

			for (uint32_t i = 0; i < DENOISE_PASSES_COUNT; ++i)
			{
				m_denoisePasses[i].passIndex = i;
				m_denoisePasses[i].subStage = this;
				m_denoisePasses[i].m_denoiseNode = m_graph->createComputeNode(countOf(slotdefsDenoiseNode), slotdefsDenoiseNode, []
				(RenderGraphNode* node, const RenderGraphNodeExecutionContext& execContext, void* usrData)
					{
						DenoisePass* denoisePass = static_cast<DenoisePass*>(usrData);
						denoisePass->subStage->executeDenoise(*denoisePass, execContext);
					},
					m_denoisePasses.data() + i, "DenoiseNode");
			}

			
		}
		m_graph->createEdge(m_clearNode, 0, m_mergeNode, 1);
		m_graph->createEdge(m_clearNode, 1, m_mergeNode, 2);
		m_graph->createEdge(m_mergeNode, 0, m_denoisePasses[0].m_denoiseNode, 2);
		m_graph->createEdge(m_mergeNode, 3, m_denoisePasses[0].m_denoiseNode, 3);

		for (uint32_t i = 0; i < m_denoisePasses.size() - 1; ++i)
		{
			m_graph->createEdge(m_denoisePasses[i].m_denoiseNode, 0, m_denoisePasses[i + 1].m_denoiseNode, 2);
			m_graph->createEdge(m_denoisePasses[i].m_denoiseNode, 1, m_denoisePasses[i + 1].m_denoiseNode, 3);

			m_graph->createEdge(m_denoisePasses[i].m_denoiseNode, 2, m_denoisePasses[i + 1].m_denoiseNode, 0);
			m_graph->createEdge(m_denoisePasses[i].m_denoiseNode, 3, m_denoisePasses[i + 1].m_denoiseNode, 1);
		}
	}

	void CombineSamplesSubStage::setInput(InputResource resource, RenderGraphNode* node, size_t slot)
	{
		switch (resource)
		{
		case InputResource::COLOR:
			m_graph->createEdge(node, slot, m_mergeNode, 4);

			//for now set to all denoise passes, really only need for first when just passing through debug stuff
			for (uint32_t i = 0; i < m_denoisePasses.size(); ++i)
			{
				m_graph->createEdge(node, slot, m_denoisePasses[i].m_denoiseNode, 6);
			}
			
			break;
		case InputResource::MATERIAL_PARAMS0:
		{
			for (uint32_t i = 0; i < m_denoisePasses.size(); ++i)
			{
				m_graph->createEdge(node, slot, m_denoisePasses[i].m_denoiseNode, 4);
			}
		}
			
			break;
		case InputResource::MATERIAL_PARAMS1:
			for (uint32_t i = 0; i < m_denoisePasses.size(); ++i)
			{
				m_graph->createEdge(node, slot, m_denoisePasses[i].m_denoiseNode, 5);
			}
			
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
			*node = m_denoisePasses.back().m_denoiseNode;
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
		for (uint32_t i = 0; i < m_denoisePasses.size(); ++i)
		{
			m_denoisePasses[i].m_denoisePassUtility.deinit();
		}
		
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
			for (uint32_t i = 0; i < m_denoisePasses.size(); ++i)
			{
				m_denoisePasses[i].m_denoisePassUtility.init(m_renderer, denoise, nullptr, 0);
				m_denoisePasses[i].m_denoiseConstants.init(data.renderGraphLifetimeResources);
			}
		}
	}
	void CombineSamplesSubStage::onRenderResolutionChanged(const RenderStage::RenderResolutionDependantResourcesData& data, uvec2 newResolution)
	{

		m_renderResolution = newResolution;

		m_mergePass.createPipelineState();
		m_clearMergeBufferPass.createPipelineState();

		for (uint32_t i = 0; i < m_denoisePasses.size(); ++i)
		{
			m_denoisePasses[i].m_denoisePassUtility.createPipelineState();
		}

		
		{
			RenderGraphResourceId resId = m_mergeNode->getRenderGraphResourceIdForSlot(0);
			const RenderGraphResourceDescription& desc = m_graph->getRenderGraphResourceDescription(resId);
			TextureDesc textureDesc(desc.resourceDimensions, desc.resourceFormat, desc.resourceUsage, m_renderResolution.x, m_renderResolution.y, 1, 1);
			TextureHandle tex = data.resolutionDependantResourcesPool->requestTexture(textureDesc, "Merged RT results");
			m_graph->setRenderGraphResourceTexture(resId, tex);
		}

		{
			RenderGraphResourceId resId = m_mergeNode->getRenderGraphResourceIdForSlot(1);
			const RenderGraphResourceDescription& desc = m_graph->getRenderGraphResourceDescription(resId);
			TextureDesc textureDesc(desc.resourceDimensions, desc.resourceFormat, desc.resourceUsage, m_renderResolution.x, m_renderResolution.y, 1, desc.arraySliceCount);
			TextureHandle tex = data.resolutionDependantResourcesPool->requestTexture(textureDesc, "Accumulation Targets");
			m_graph->setRenderGraphResourceTexture(resId, tex);
		}

		{
			RenderGraphResourceId resId = m_mergeNode->getRenderGraphResourceIdForSlot(2);
			const RenderGraphResourceDescription& desc = m_graph->getRenderGraphResourceDescription(resId);
			TextureDesc textureDesc(desc.resourceDimensions, desc.resourceFormat, desc.resourceUsage, m_renderResolution.x, m_renderResolution.y, 1, 1);
			TextureHandle tex = data.resolutionDependantResourcesPool->requestTexture(textureDesc, "Sample Count");
			m_graph->setRenderGraphResourceTexture(resId, tex);
		}

		{
			RenderGraphResourceId resId = m_mergeNode->getRenderGraphResourceIdForSlot(3);
			const RenderGraphResourceDescription& desc = m_graph->getRenderGraphResourceDescription(resId);
			TextureDesc textureDesc(desc.resourceDimensions, desc.resourceFormat, desc.resourceUsage, m_renderResolution.x, m_renderResolution.y, 1, 1);
			TextureHandle tex = data.resolutionDependantResourcesPool->requestTexture(textureDesc, "Variance");
			m_graph->setRenderGraphResourceTexture(resId, tex);
		}

		{
			RenderGraphResourceId resId = m_denoisePasses[0].m_denoiseNode->getRenderGraphResourceIdForSlot(0);
			const RenderGraphResourceDescription& desc = m_graph->getRenderGraphResourceDescription(resId);
			TextureDesc textureDesc(desc.resourceDimensions, desc.resourceFormat, desc.resourceUsage, m_renderResolution.x, m_renderResolution.y, 1, 1);
			TextureHandle tex = data.resolutionDependantResourcesPool->requestTexture(textureDesc, "denoised RT result");
			m_graph->setRenderGraphResourceTexture(resId, tex);
		}
		
		{
			RenderGraphResourceId resId = m_denoisePasses[0].m_denoiseNode->getRenderGraphResourceIdForSlot(1);
			const RenderGraphResourceDescription& desc = m_graph->getRenderGraphResourceDescription(resId);
			TextureDesc textureDesc(desc.resourceDimensions, desc.resourceFormat, desc.resourceUsage, m_renderResolution.x, m_renderResolution.y, 1, 1);
			TextureHandle tex = data.resolutionDependantResourcesPool->requestTexture(textureDesc, "Variance2");
			m_graph->setRenderGraphResourceTexture(resId, tex);
		}
		
	}

	void CombineSamplesSubStage::update(const UpdateParams& params)
	{
		bool haveTexturesChanged = m_graph->isResourceBoundThisFrame(m_mergeNode->getRenderGraphResourceIdForSlot(0));
		haveTexturesChanged = haveTexturesChanged || m_graph->isResourceBoundThisFrame(m_mergeNode->getRenderGraphResourceIdForSlot(1));
		haveTexturesChanged = haveTexturesChanged || m_graph->isResourceBoundThisFrame(m_denoisePasses[0].m_denoiseNode->getRenderGraphResourceIdForSlot(0));
		haveTexturesChanged = haveTexturesChanged || m_graph->isResourceBoundThisFrame(m_denoisePasses[0].m_denoiseNode->getRenderGraphResourceIdForSlot(1));
		haveTexturesChanged = haveTexturesChanged || m_graph->isResourceBoundThisFrame(m_denoisePasses[0].m_denoiseNode->getRenderGraphResourceIdForSlot(2));
		haveTexturesChanged = haveTexturesChanged || m_graph->isResourceBoundThisFrame(m_denoisePasses[0].m_denoiseNode->getRenderGraphResourceIdForSlot(3));
		haveTexturesChanged = haveTexturesChanged || m_graph->isResourceBoundThisFrame(m_denoisePasses[0].m_denoiseNode->getRenderGraphResourceIdForSlot(4));
		haveTexturesChanged = haveTexturesChanged || m_graph->isResourceBoundThisFrame(m_denoisePasses[0].m_denoiseNode->getRenderGraphResourceIdForSlot(5));
		haveTexturesChanged = haveTexturesChanged || m_graph->isResourceBoundThisFrame(m_denoisePasses[0].m_denoiseNode->getRenderGraphResourceIdForSlot(6));

		CRendererConfiguration& config = m_renderer->getConcreteRendererConfiguration();
		int32_t denoiseMode = config.getRendererVarValueInternal<int32_t>(RVARNAME_DENOISE_MODE);
		float depthSigmaScale = config.getRendererVarValueInternal<float>(RVARNAME_DENOISE_DEPTH_SIGMA_SCALE);
		float normalSigma = config.getRendererVarValueInternal<float>(RVARNAME_DENOISE_NORMAL_SIGMA);
		float matDiffSigma = config.getRendererVarValueInternal<float>(RVARNAME_DENOISE_MATERIAL_DIFFERENCE_SIGMA);
		float colorDiffSigma = config.getRendererVarValueInternal<float>(RVARNAME_DENOISE_COLOR_DIFFERENCE_SIGMA);
		
		float fireflyClampScale = config.getRendererVarValueInternal<float>(RVARNAME_FIREFLY_CLAMP_SCALE);
		float fireflyClampRelaxPower = config.getRendererVarValueInternal<float>(RVARNAME_FIREFLY_CLAMP_RELAX_POWER);

		bool disableAccum = config.getRendererVarValueInternal<int32_t>(RVARNAME_ACCUMULATION_DISABLE) != 0;

		if (haveTexturesChanged)
		{
			//clear accumulation buffer
			{
				TextureViewHandle accumulationTargets = m_graph->getTextureViewFromNodeSlot(m_clearNode->getSortedIndex(), 0);
				TextureViewHandle sampleCountTarget = m_graph->getTextureViewFromNodeSlot(m_clearNode->getSortedIndex(), 1);


				DescriptorSetUpdate updates[] = {
					{0, 0, 1, DescriptorPtr(m_clearMergeBufferConstants.getViewPtr())},
					{1, 0, 1, DescriptorPtr(&accumulationTargets)},
					{2, 0, 1, DescriptorPtr(&sampleCountTarget)}
				};
				m_clearMergeBufferPass.reserveAndUpdateDescriptorSet(0, updates, countOf(updates));
			}

			//Merge
			{
				TextureViewHandle mergeTarget = m_graph->getTextureViewFromNodeSlot(m_mergeNode->getSortedIndex(), 0);
				TextureViewHandle accumulationTargets = m_graph->getTextureViewFromNodeSlot(m_mergeNode->getSortedIndex(), 1);
				TextureViewHandle sampleCountTarget = m_graph->getTextureViewFromNodeSlot(m_mergeNode->getSortedIndex(), 2);
				TextureViewHandle varianceTarget = m_graph->getTextureViewFromNodeSlot(m_mergeNode->getSortedIndex(), 3);
				TextureViewHandle newSamples = m_graph->getTextureViewFromNodeSlot(m_mergeNode->getSortedIndex(), 4);

				DescriptorSetUpdate updates[] = {
					{0, 0, 1, DescriptorPtr(m_mergeSamplesConstants.getViewPtr())},
					{1, 0, 1, DescriptorPtr(&mergeTarget)},
					{2, 0, 1, DescriptorPtr(&accumulationTargets)},
					{3, 0, 1, DescriptorPtr(&sampleCountTarget)},
					{4, 0, 1, DescriptorPtr(&varianceTarget) },
					{5, 0, 1, DescriptorPtr(&newSamples)},
				};
				m_mergePass.reserveAndUpdateDescriptorSet(0, updates, countOf(updates));
			}

			//denoise
			{
				for (uint32_t i = 0; i < m_denoisePasses.size(); ++i)
				{
					ComputeNode* denoiseNode = m_denoisePasses[i].m_denoiseNode;
					TextureViewHandle denoiseTarget = m_graph->getTextureViewFromNodeSlot(denoiseNode->getSortedIndex(), 0);
					TextureViewHandle denoiseTargetVar = m_graph->getTextureViewFromNodeSlot(denoiseNode->getSortedIndex(), 1);
					TextureViewHandle denoiseSource = m_graph->getTextureViewFromNodeSlot(denoiseNode->getSortedIndex(), 2);
					TextureViewHandle varianceTarget = m_graph->getTextureViewFromNodeSlot(denoiseNode->getSortedIndex(), 3);
					TextureViewHandle matParams0 = m_graph->getTextureViewFromNodeSlot(denoiseNode->getSortedIndex(), 4);
					TextureViewHandle matParams1 = m_graph->getTextureViewFromNodeSlot(denoiseNode->getSortedIndex(), 5);
					TextureViewHandle latestSamples = m_graph->getTextureViewFromNodeSlot(denoiseNode->getSortedIndex(), 6);

					DescriptorSetUpdate updates[] = {
						{0, 0, 1, DescriptorPtr(m_denoisePasses[i].m_denoiseConstants.getViewPtr())},
						{1, 0, 1, DescriptorPtr(&denoiseSource)},
						{2, 0, 1, DescriptorPtr(&matParams0)},
						{3, 0, 1, DescriptorPtr(&matParams1)},
						{4, 0, 1, DescriptorPtr(&varianceTarget)},
						{5, 0, 1, DescriptorPtr(&latestSamples) },
						{6, 0, 1, DescriptorPtr(&denoiseTarget) },
						{7, 0, 1, DescriptorPtr(&denoiseTargetVar) }

					};
					m_denoisePasses[i].m_denoisePassUtility.reserveAndUpdateDescriptorSet(0, updates, countOf(updates));
				}
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
				params->accumulationTargetCount = ACCUMULATION_TARGET_COUNT;
				m_clearMergeBufferConstants.flush();
			}

			{
				MergeNewSamplesParams* mergeSamplesParams = m_mergeSamplesConstants.getData();
				mergeSamplesParams->sourceTextureDimensions = glm::uvec2(params.sourceTextureResolution.x, params.sourceTextureResolution.y);
				mergeSamplesParams->targetTextureOffsetScaleBias = params.targetOffsetScaleBias;
				mergeSamplesParams->targetTextureDimensions = params.targetTextureResolution;
				mergeSamplesParams->sampleCount = params.samplesPerPixel;
				mergeSamplesParams->accumulationTargetCount = ACCUMULATION_TARGET_COUNT;
				mergeSamplesParams->fireflyClampScale = fireflyClampScale;
				mergeSamplesParams->fireflyClampRelaxPower = fireflyClampRelaxPower;
				MathUtils::generateSobolSequence(1u, 4u, glm::value_ptr(mergeSamplesParams->randomSequence), (uint32_t)params.samplesPerPixel);
				m_mergeSamplesConstants.flush();
			}

			vec4 camPos(0.f, 0.f, 0.f, 1.f);
			mat4 worldToView = m_renderer->getCurrentRenderView().getView();
			mat4 viewToWorld = glm::inverse(worldToView);
			camPos = viewToWorld * camPos;

			mat4 viewToUVTransform = fromPlatformNDCToTextureSpace() * m_renderer->getCurrentRenderView().getProjectionPlatform();
			mat4 uvToViewTransform = glm::inverse(viewToUVTransform);

			for (uint32_t i = 0; i < m_denoisePasses.size(); ++i)
			{
				DenoiseParams* denoiseParams = m_denoisePasses[i].m_denoiseConstants.getData();
				denoiseParams->modePassIndex = uvec4p(denoiseMode, i, 0, 0);
				denoiseParams->textureDimensions = vec4p(m_renderResolution.x, m_renderResolution.y, targetPixelWidth, targetPixelHeight);

				
				denoiseParams->uvToView = uvToViewTransform;
				denoiseParams->viewToWorld = viewToWorld;
				denoiseParams->cameraPositionWS = camPos;
				denoiseParams->bilaterWeightParams = vec4p(depthSigmaScale, normalSigma, matDiffSigma, colorDiffSigma);
				denoiseParams->miscParams = uvec4p(i == m_denoisePasses.size() - 1 ? 1: 0, 0, 0, 0);

				m_denoisePasses[i].m_denoiseConstants.flush();
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


	void CombineSamplesSubStage::executeDenoise(DenoisePass& pass, const RenderGraphNodeExecutionContext& exec)
	{

		uint32_t rtWidth = m_lastUpdateParams.targetTextureResolution.x;
		uint32_t rtHeight = m_lastUpdateParams.targetTextureResolution.y;

		uint32_t dispatchX = (rtWidth + DENOISE_WG_SIZE - 1) / DENOISE_WG_SIZE;
		uint32_t dispatchY = (rtHeight + DENOISE_WG_SIZE - 1) / MERGE_SAMPLES_WG_SIZE;

		pass.m_denoisePassUtility.dispatch(exec.cmdBuffer, dispatchX, dispatchY, 1);
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