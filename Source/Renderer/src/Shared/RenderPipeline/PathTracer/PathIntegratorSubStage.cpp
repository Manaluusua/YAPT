#include <Renderer/Shared/RenderPipeline/PathTracer/PathIntegratorSubStage.h>
#include <Gfx/RenderGraph/RenderGraph.h>

#include <Renderer/Shared/BindlessTextureManager.h>
#include <Renderer/Shared/BindlessBufferManager.h>
#include <Renderer/Shared/Utility/CoreRenderResourcesUtility.h>
#include <Math/RandUtility.h>
#include <Math/MathUtility.h>
#include <Gfx/GfxBasicTypesUtility.h>
#include <Renderer/Shared/TextureImpl.h>
#include <Renderer/Shared/BufferImpl.h>
#include <Math/RandUtility.h>
#include <Math/MathUtility.h>
#include <Renderer/Shared/Utility/RenderAPIAbstractionUtility.h>
#include <Renderer/Shared/BindlessMaterialManager.h>
#include <Renderer/Shared/BindlessMeshManager.h>

namespace YAPT
{
	const size_t ENVIRONMENT_TYPE_NONE = 0;
	const size_t ENVIRONMENT_TYPE_CUBE = 1;
	const size_t ENVIRONMENT_TYPE_LONGLAT = 2;

	PathIntegratorSubStage::PathIntegratorSubStage()
		:m_raytracePso(YAPT_NULL_HANDLE),
		m_rtDescSet(YAPT_NULL_HANDLE),
		m_applySubpixelJitter(true)
		
	{
		initSubpixelJitterSamples();
	}
	PathIntegratorSubStage::~PathIntegratorSubStage()
	{

	}

	void PathIntegratorSubStage::initialize(AccelerationStructureProvider* accStructProvider, CRenderer* rend, RenderGraph* graph, BindlessMaterialManager* matMngr, BindlessMeshManager* meshMngr)
	{
		m_graph = graph;
		m_renderer = rend;
		m_accStructProvider = accStructProvider;
		m_materialMngr = matMngr;
		m_meshMngr = meshMngr;

		RenderGraphNodeSlotDefinition slotdefsRtNode[] =
		{ {ResourceDimension::TEXTURE_2D,
			ResourceFormat::RGBA16_SFLOAT,
			RESOURCE_USAGE_STORAGE_TEXTURE,
			ACCESS_FLAGS_READ_WRITE,
			SHADERSTAGE_RT_RAYGENERATION,
			1,
			1}
		};


		m_rtNode = m_graph->createRayTraceNode(1, slotdefsRtNode, []
		(RenderGraphNode* node, const RenderGraphNodeExecutionContext& execContext, void* usrData)
			{
				static_cast<PathIntegratorSubStage*>(usrData)->executeRaytrace(execContext);
			},
			this, "RaytraceNode");
	}
	void PathIntegratorSubStage::shutdown()
	{
		if (m_raytracePso != YAPT_NULL_HANDLE)
		{
			Gfx::destroyRaytracePipelineState(m_renderer->getGfxHandle(), m_raytracePso);
			m_raytracePso = YAPT_NULL_HANDLE;
		}

		m_shaderTableHelper.deinit();
	}
	void PathIntegratorSubStage::onRenderGraphCompiled(const RenderStage::RenderGraphLifetimeData& data)
	{
		ShaderLoader* loader = m_renderer->getShaderLoader();
		const ShaderLoader::ShaderPipelineInfo* rayGen = loader->getShaderPipeline("rayGenPrimaryRays");
		const ShaderLoader::ShaderPipelineInfo* rayHit = loader->getShaderPipeline("rayHitDefault");
		const ShaderLoader::ShaderPipelineInfo* rayMiss = loader->getShaderPipeline("rayMissEnvironment");

		ShaderPipelineReflection* reflArray[] = { rayGen->reflection, rayHit->reflection, rayMiss->reflection };
		m_rtLayout.initRaytraceLayoutFromShaderReflections(m_renderer->getGfxHandle(), reflArray, countOf(reflArray));

		//texture and buffer array setup
		{
			BindlessTextureManager* texMngr = m_renderer->getTextureManager();
			BindlessBufferManager* buffMngr = m_renderer->getBufferManager();
			const DescriptorSetLayoutBinding& bindingTex = texMngr->getBindingDefinition();
			const DescriptorSetLayoutBinding& bindingBuff = buffMngr->getBindingDefinition();
			m_rtLayout.setExplicitDescriptorSetLayout(1, &bindingTex, 1, texMngr->getLayout());
			m_rtLayout.setExplicitDescriptorSetLayout(2, &bindingBuff, 1, buffMngr->getLayout());

		}


		//TODO: just go through all the pipelines and map the predefined static samplers
		{
			ShaderPipelineReflection::NameMapping nameMapping;
			{
				bool found = rayMiss->reflection->getNameMapping(ShaderModuleType::LIBRARY_MODULE, "g_colorSampler", nameMapping);
				if (found)
				{
					SamplerHandle samplerHandle = m_renderer->getCoreResources()->getDefaultSampler(DefaultSamplerType::LINEAR_REPEAT);
					m_rtLayout.setStaticSamplers(nameMapping, &samplerHandle);
				}
			}

			{
				bool found = rayHit->reflection->getNameMapping(ShaderModuleType::LIBRARY_MODULE, "g_colorSampler", nameMapping);
				if (found)
				{
					SamplerHandle samplerHandle = m_renderer->getCoreResources()->getDefaultSampler(DefaultSamplerType::NEAREST_REPEAT);
					m_rtLayout.setStaticSamplers(nameMapping, &samplerHandle);
				}
			}

			{
				bool found = rayHit->reflection->getNameMapping(ShaderModuleType::LIBRARY_MODULE, "g_pointSampler", nameMapping);
				if (found)
				{
					SamplerHandle samplerHandle = m_renderer->getCoreResources()->getDefaultSampler(DefaultSamplerType::NEAREST_REPEAT);
					m_rtLayout.setStaticSamplers(nameMapping, &samplerHandle);
				}
			}

			{
				bool found = rayHit->reflection->getNameMapping(ShaderModuleType::LIBRARY_MODULE, "g_lutSampler", nameMapping);
				if (found)
				{
					SamplerHandle samplerHandle = m_renderer->getCoreResources()->getDefaultSampler(DefaultSamplerType::LINEAR_CLAMP);
					m_rtLayout.setStaticSamplers(nameMapping, &samplerHandle);
				}
			}


		}

		m_rtLayout.compile();


		if (m_raytracePso != YAPT_NULL_HANDLE)
		{
			Gfx::destroyRaytracePipelineState(m_renderer->getGfxHandle(), m_raytracePso);
		}
		RaytracePipelineStateDesc desc;
		RayTraceShaderConfig config;

		ShaderStageCreateInfo shaderStages[3];


		fillShaderModuleCreateInfo(rayGen->shaderModules[0], shaderStages[0]);
		fillShaderModuleCreateInfo(rayHit->shaderModules[0], shaderStages[1]);
		fillShaderModuleCreateInfo(rayMiss->shaderModules[0], shaderStages[2]);

		RayGenerationDescription rayGenDescs[1] = { {0, 0} };
		RayHitGroupDescription hitGroupDescs[] = { {"defaultHitGroup", 1, YAPT_NULL_INDEX, YAPT_NULL_INDEX, 0, HitGroupType::TRIANGLE} };
		RayMissDescription rayMissDescs[1] = { {2, 0} };;

		config.maxAttributeSizeInBytes = sizeof(float) * 2;
		config.maxPayloadSizeInBytes = sizeof(RaytracePayload);

		desc.numberOfShaders = countOf(shaderStages);
		desc.shaders = shaderStages;

		desc.layout = m_rtLayout.getPipelineLayoutHandle();

		desc.configs = &config;
		desc.numberOfConfigs = 1;

		desc.hitGroupDescriptions = hitGroupDescs;
		desc.numberOfHitGroupDescription = countOf(hitGroupDescs);
		desc.hitGroupShaderTableConstantsSizeInBytes = sizeof(RayHitShaderTableConstantData);

		desc.rayGenerationDescriptions = rayGenDescs;
		desc.numberOfRayGenerationDescription = countOf(rayGenDescs);
		desc.missShaderTableConstantsSizeInBytes = sizeof(RayMissShaderTableConstantData);

		desc.rayMissDescriptions = rayMissDescs;
		desc.numberOfRayMissDescription = countOf(rayMissDescs);
		desc.rayGenShaderTableConstantsSizeInBytes = 0;

		desc.maxTraceRecursionDepth = 2;

		m_raytracePso = Gfx::createRaytracePipelineState(m_renderer->getGfxHandle(), desc);

		m_shaderTableHelper.init(m_renderer, m_raytracePso, desc.rayGenShaderTableConstantsSizeInBytes,
			desc.missShaderTableConstantsSizeInBytes, desc.hitGroupShaderTableConstantsSizeInBytes);


		m_rayTraceConstants.init(data.renderGraphLifetimeResources);
		m_randomSamples.init(data.renderGraphLifetimeResources);
		m_spectralDataConstants.init(data.renderGraphLifetimeResources);
	}
	void PathIntegratorSubStage::onRenderResolutionChanged(const RenderStage::RenderResolutionDependantResourcesData& data, uvec2 newResolution)
	{
		m_renderResolution = newResolution;

		RenderGraphResourceId rtTarget = m_rtNode->getRenderGraphResourceIdForSlot(0);
		const RenderGraphResourceDescription& desc = m_graph->getRenderGraphResourceDescription(rtTarget);

		TextureDesc textureDesc(desc.resourceDimensions, desc.resourceFormat, desc.resourceUsage, newResolution.x, newResolution.y, 1, 1);
		TextureHandle rtTargetTex = data.resolutionDependantResourcesPool->requestTexture(textureDesc, "Main scene RT Color Target");
		m_graph->setRenderGraphResourceTexture(rtTarget, rtTargetTex);
		
	}

	void PathIntegratorSubStage::update(const PathIntegratorSubStage::UpdateParams& params)
	{
		RaytraceConstantData* rtConstants = m_rayTraceConstants.getData();
		float targetPixelWidth = 1.f / m_renderResolution.x;
		float targetPixelHeight = 1.f / m_renderResolution.y;

		//ray offset
		vec2p rayUVOffset(0.5 * targetPixelWidth, 0.5 * targetPixelHeight); //move to pixel center
		rayUVOffset += params.rayGenOffsetInTexels; //move to (target) pixel being updated (will be 0 if doing fullres rt)

		//apply subpixel jitter
		if (m_applySubpixelJitter)
		{
			uint32_t subpixelJitterSampleIndex = params.sampleOffset % NUMBER_OF_SUBPIXEL_JITTER_SAMPLES;
			vec2p jitterSample = m_subpixelJitterSamples[subpixelJitterSampleIndex];
			rayUVOffset.x += jitterSample.x * targetPixelWidth;
			rayUVOffset.y += jitterSample.y * targetPixelHeight;
		}

		vec4 camPos(0.f, 0.f, 0.f, 1.f);
		mat4 viewToWorld = glm::inverse(m_renderer->getCurrentRenderView().getView());
		camPos = viewToWorld * camPos;

		rtConstants->currentSampleIndex = uint32_t(params.sampleOffset % 0xFFFFFFFF);
		rtConstants->maxRayDepth = 16;
		rtConstants->rayUVOffset = rayUVOffset;
		rtConstants->cameraPosition = camPos;

		mat4 uvToViewTransform = glm::inverse(fromPlatformNDCToTextureSpace() * m_renderer->getCurrentRenderView().getProjectionPlatform());

		rtConstants->uvToView = uvToViewTransform;
		rtConstants->viewToWorld = viewToWorld;

		//update samples
		updateSampledWavelengths(params.sampleOffset);
		if ((params.sampleOffset % NUMBER_OF_RANDOM_SAMPLES) == 0)
		{
			updateSamples(params.sampleOffset);
		}

		m_rayTraceConstants.flush();
		
		m_lastUpdateParams = params;
	}

	void PathIntegratorSubStage::getOutput(RenderGraphNode** node, size_t& slotOut)
	{
		*node = m_rtNode;
		slotOut = 0;
	}

	//stride and offset are assumed in dwords (uint32/float32) in the shader
	uvec2p PathIntegratorSubStage::packBufferInfo(uint32_t bufferIndex, uint32_t bufferStride, uint32_t bufferOffset)
	{
		glm::uvec2 v;
		v.x = bufferOffset;
		v.y = bufferStride << 16 | (bufferIndex & 0xFFFF);
		return v;
	}

	void PathIntegratorSubStage::writeShaderTableEntryAndConstantData(size_t shaderTableIndex, const MaterialPerSubmeshArray& mat, const MeshInternal* mesh, size_t submeshIndex, ShaderTableEntry* entry)
	{

		size_t matId = mat.getMaterialForSubmeshIndex(submeshIndex)->getID();
		size_t meshId = mesh->getMeshIndex();

		size_t matIndex = m_materialMngr->getEntryIndexForMaterialId(matId);
		size_t meshIndex = m_meshMngr->getEntryIndexForMeshIdAndSubmesh(meshId, submeshIndex);

		RayHitShaderTableConstantData rayHitConstants;
		rayHitConstants.materialAndMeshIndices = uvec2p(matIndex, meshIndex);


		entry->extraDataInBytes = sizeof(RayHitShaderTableConstantData);
		memcpy(entry->shaderTableExtraData, &rayHitConstants, sizeof(RayHitShaderTableConstantData));

		entry->shaderIndexInPso = 0;
		entry->shaderTableIndex = shaderTableIndex;
	}

	void PathIntegratorSubStage::updateShaderTable(const RenderObjectId* ids, MaterialPerSubmeshArray* materials, MeshInternal** meshes, size_t* instanceOffsets, size_t objectCount, size_t instancesCount)
	{

		bool emptyHitGroup = instancesCount == 0;

		m_shaderTableHelper.resize(1, 1, max((size_t)1, instancesCount));

		for (size_t i = 0; i < objectCount; ++i)
		{
			size_t shaderTableOffset = instanceOffsets[i];
			for (size_t k = 0; k < meshes[i]->getSubmeshCount(); ++k)
			{
				ShaderTableEntry* entry = m_shaderTableHelper.appendHitGroupUpdate();
				writeShaderTableEntryAndConstantData(shaderTableOffset + k, materials[i], meshes[i], k, entry);
			}

		}

		if (emptyHitGroup)
		{
			ShaderTableEntry* entry = m_shaderTableHelper.appendHitGroupUpdate();
			entry->shaderIndexInPso = 0;
			entry->shaderTableIndex = 0;
			entry->extraDataInBytes = 0;
		}


		//update rayGen
		{
			ShaderTableEntry* entry = m_shaderTableHelper.appendRayGenUpdate();
			entry->shaderIndexInPso = 0;
			entry->shaderTableIndex = 0;
			entry->extraDataInBytes = 0;
		}


		//update miss
		{
			RCPtr<Texture> skymap = m_renderer->getConcreteRendererConfiguration().getRendererVarValueInternal<Texture*>(RVARNAME_SKYBOX);
			RayMissShaderTableConstantData missConstantData;

			bool hasValidEnvtex = false;

			if (skymap != nullptr)
			{
				if (skymap->getDesc().dimension == ResourceDimension::TEXTURE_CUBEMAP)
				{
					hasValidEnvtex = true;
					missConstantData.envType = ENVIRONMENT_TYPE_CUBE;

				}
				else if (skymap->getDesc().dimension == ResourceDimension::TEXTURE_2D)
				{
					hasValidEnvtex = true;
					missConstantData.envType = ENVIRONMENT_TYPE_LONGLAT;
				}
			}

			if (hasValidEnvtex)
			{
				missConstantData.envTextureIndex = static_cast<TextureImpl*>(skymap.get())->getBindlessResourceArrayIndex();
			}
			else
			{
				missConstantData.envType = ENVIRONMENT_TYPE_NONE;
				missConstantData.envTextureIndex = uint32_t(-1);
			}

			ShaderTableEntry* entry = m_shaderTableHelper.appendMissShaderUpdate();
			entry->shaderIndexInPso = 0;
			entry->shaderTableIndex = 0;
			entry->extraDataInBytes = sizeof(RayMissShaderTableConstantData);
			memcpy(entry->shaderTableExtraData, &missConstantData, sizeof(RayMissShaderTableConstantData));
		}


		m_shaderTableHelper.flush();
	}

	void PathIntegratorSubStage::updateSamples(size_t sampleOffset)
	{
		MathUtils::generateHaltonSequence(NUMBER_OF_RANDOM_SAMPLES, m_randomSamples.getData()->samples, sampleOffset);
		m_randomSamples.flush();
	}

	void PathIntegratorSubStage::updateSampledWavelengths(size_t sampleOffset)
	{
		SpectralDataConstants* data = m_spectralDataConstants.getData();
		if ((sampleOffset % SPECTRAL_SAMPLESET_COUNT) == 0)
		{
			std::array<float, SPECTRAL_SAMPLESET_COUNT* SPECTRAL_SAMPLES_COUNT> sampleLambdas;
			std::array<float, SPECTRAL_SAMPLESET_COUNT* SPECTRAL_SAMPLES_COUNT> samplePDFs;

			for (uint32_t i = 0; i < SPECTRAL_SAMPLESET_COUNT; ++i)
			{
				float rand = MathUtils::halton<float>(11, sampleOffset + i);
				SpectralUtility::generateSampleLambdas(rand, (size_t)SPECTRAL_SAMPLES_COUNT, sampleLambdas.data() + i * SPECTRAL_SAMPLES_COUNT, samplePDFs.data() + i * SPECTRAL_SAMPLES_COUNT, (float)SpectralUtility::getCIELUTMinLambda(), (float)SpectralUtility::getCIELUTMaxLambda());
			}

			memcpy(&data->spdSampleLambda, sampleLambdas.data(), sizeof(float) * sampleLambdas.size());
			memcpy(&data->spdSamplePdf, samplePDFs.data(), sizeof(float) * samplePDFs.size());

		}

		data->sampleSetOffset = sampleOffset % SPECTRAL_SAMPLESET_COUNT;
		m_spectralDataConstants.flush();
	}

	void PathIntegratorSubStage::initSubpixelJitterSamples()
	{
		MathUtils::generateHaltonSequence(NUMBER_OF_SUBPIXEL_JITTER_SAMPLES, m_subpixelJitterSamples, 0);
		//[0,1] -> [-0.5, 0.5], ie offsets from pixel center
		for (size_t i = 0; i < NUMBER_OF_SUBPIXEL_JITTER_SAMPLES; ++i)
		{
			m_subpixelJitterSamples[i] = (m_subpixelJitterSamples[i] - vec2p(0.5f, 0.5f));
		}

	}


	void PathIntegratorSubStage::executeRaytrace(const RenderGraphNodeExecutionContext& exec)
	{
		//we will create the descset here since we need to acceleration structures done before this
		{
			if (m_rtDescSet != YAPT_NULL_HANDLE)
			{
				m_rtLayout.getDescriptorSetUtility(0).freeDescriptorSet(m_rtDescSet);
			}

			m_rtDescSet = m_rtLayout.getDescriptorSetUtility(0).getNewDescriptorSet();
			TextureViewHandle rtOutputUav = m_graph->getTextureViewFromNodeSlot(m_rtNode->getSortedIndex(), 0);
			TopLevelAccelerationStructureHandle accStruct = m_accStructProvider->getAccelerationStructure(exec.cmdBuffer);

			BufferViewHandle meshEntriesBuffer = m_meshMngr->getBufferViewHandle();
			BufferViewHandle materialEntriesBuffer = m_materialMngr->getBufferViewHandle();

			TextureViewHandle noiseTex = m_renderer->getCoreResources()->getDefaultTextureView(DefaultTextureType::NOISE);

			TextureViewHandle singleScatterAlbedoNoFresnel = m_renderer->getCoreResources()->getMultiScatteringLUTs().getSingleScatterDirectionalAlbedoNoFresnel();
			TextureViewHandle singleScatterAverageAlbedoNoFresnel = m_renderer->getCoreResources()->getMultiScatteringLUTs().getSingleScatterAverageDirectionalAlbedoNoFresnel();
			TextureViewHandle singleAndMultiScatterAlbedo = m_renderer->getCoreResources()->getMultiScatteringLUTs().getSingleAndMultiScatterDirectionalAlbedo();
			TextureViewHandle singleAndMultiScatterAverageAlbedo = m_renderer->getCoreResources()->getMultiScatteringLUTs().getSingleAndMultiScatterAverageDirectionalAlbedo();
			TextureViewHandle singleScatterAlbedoTranslucentDenser = m_renderer->getCoreResources()->getMultiScatteringLUTs().getSingleScatterDirectionalAlbedoTranslucentToDenser();
			TextureViewHandle singleScatterAverageAlbedoTranslucentLighter = m_renderer->getCoreResources()->getMultiScatteringLUTs().getSingleScatterDirectionalAlbedoTranslucentToLighter();

			TextureViewHandle singleScatterAvgAlbedoTranslucentDenser = m_renderer->getCoreResources()->getMultiScatteringLUTs().getSingleScatterAverageAlbedoTranslucentToDenser();
			TextureViewHandle singleScatterAvgAverageAlbedoTranslucentLighter = m_renderer->getCoreResources()->getMultiScatteringLUTs().getSingleScatterAverageAlbedoTranslucentToLighter();

			TextureViewHandle sheenDirectionalAlbedo = m_renderer->getCoreResources()->getMultiScatteringLUTs().getDirectionalAlbedoSheen();

			TextureViewHandle cieLUT = m_renderer->getCoreResources()->getSpectralUtility().getXYZColorMatchingLUT();
			TextureViewHandle d65LUT = m_renderer->getCoreResources()->getSpectralUtility().getD65IlluminantLUT();
			TextureViewHandle toSRGBLUT = m_renderer->getCoreResources()->getSpectralUtility().getSRGBToSPDLUT();
			TextureViewHandle toREC2020LUT = m_renderer->getCoreResources()->getSpectralUtility().getREC2020ToSPDLUT();


			DescriptorSetUpdate updates[] = {
				{1, 0, 1, DescriptorPtr(m_rayTraceConstants.getViewPtr())},
				{2, 0, 1, DescriptorPtr(m_randomSamples.getViewPtr())},
				{3, 0, 1, DescriptorPtr(m_spectralDataConstants.getViewPtr())},
				{4, 0, 1, DescriptorPtr(&materialEntriesBuffer)},
				{5, 0, 1, DescriptorPtr(&meshEntriesBuffer)},
				
				{6, 0, 1, DescriptorPtr(&accStruct) },
				{7, 0, 1, DescriptorPtr(&rtOutputUav)},
				{11, 0, 1, DescriptorPtr(&noiseTex)},

				{12, 0, 1, DescriptorPtr(&singleScatterAlbedoNoFresnel)},
				{13, 0, 1, DescriptorPtr(&singleScatterAverageAlbedoNoFresnel)},
				{14, 0, 1, DescriptorPtr(&singleAndMultiScatterAlbedo)},
				{15, 0, 1, DescriptorPtr(&singleAndMultiScatterAverageAlbedo)},
				{16, 0, 1, DescriptorPtr(&singleScatterAlbedoTranslucentDenser)},
				{17, 0, 1, DescriptorPtr(&singleScatterAverageAlbedoTranslucentLighter)},
				{18, 0, 1, DescriptorPtr(&singleScatterAvgAlbedoTranslucentDenser)},
				{19, 0, 1, DescriptorPtr(&singleScatterAvgAverageAlbedoTranslucentLighter)},
				{20, 0, 1, DescriptorPtr(&sheenDirectionalAlbedo)},

				{21, 0, 1, DescriptorPtr(&cieLUT)},
				{22, 0, 1, DescriptorPtr(&d65LUT)},
				{23, 0, 1, DescriptorPtr(&toREC2020LUT)},
				{24, 0, 1, DescriptorPtr(&toSRGBLUT)},

			};
			Gfx::updateDescriptorSet(m_renderer->getGfxHandle(), m_rtDescSet, updates, countOf(updates));
		}

		BindlessTextureManager* texMngr = m_renderer->getTextureManager();
		BindlessBufferManager* buffMngr = m_renderer->getBufferManager();
		GfxApiHandle gfx = m_renderer->getGfxHandle();
		Gfx::setRaytracePipelineState(gfx, exec.cmdBuffer, m_raytracePso);

		DescriptorSetHandle bindings[] = {
			 m_rtDescSet,
			 texMngr->getTextureArrayDescSet(),
			 buffMngr->getBufferArrayDescSet()
		};
		Gfx::bindDescriptorSets(gfx, exec.cmdBuffer, BindingPoint::BINDING_POINT_RAYTRACE, m_rtLayout.getPipelineLayoutHandle(), bindings, 0, countOf(bindings), nullptr, 0);

		Gfx::dispatchRays(gfx, exec.cmdBuffer, m_lastUpdateParams.raysPerFrame.x, m_lastUpdateParams.raysPerFrame.y, 1, m_shaderTableHelper.getShaderTable());
	}

}