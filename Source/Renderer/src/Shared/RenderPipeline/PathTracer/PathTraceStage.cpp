#include <Renderer/Shared/RenderPipeline/PathTracer/PathTraceStage.h>
#include <Gfx/RenderGraph/RenderGraph.h>
#include <Renderer/Shared/Utility/RenderResourcesPool.h>
#include <Renderer/Shared/Utility/PipelineStateDescriptionUtility.h>
#include <Renderer/Shared/CRenderer.h>
#include <Renderer/Shared/Utility/CoreRenderResourcesUtility.h>
#include <Renderer/Shared/Utility/SpectralUtility.h>
#include <Renderer/Shared/Utility/RenderAPIAbstractionUtility.h>
#include <Renderer/Shared/BindlessTextureManager.h>
#include <Renderer/Shared/BindlessBufferManager.h>
#include <Math/RandUtility.h>
#include <Math/MathUtility.h>
#include <Gfx/GfxBasicTypesUtility.h>

#include <Renderer/Shared/TextureImpl.h>
#include <Renderer/Shared/BufferImpl.h>



using namespace YAPT::MathUtils;

namespace YAPT
{
	const size_t ENVIRONMENT_TYPE_NONE = 0;
	const size_t ENVIRONMENT_TYPE_CUBE = 1;
	const size_t ENVIRONMENT_TYPE_LONGLAT = 2;

	PathTraceStage::PathTraceStage()
		:m_raytracePso(YAPT_NULL_HANDLE),
		m_rtDescSet(YAPT_NULL_HANDLE),
		m_resolveTargetWidth(0),
		m_resolveTargetHeight(0),
		m_raysPerFrameDivisor(1),
		m_applySubpixelJitter(true),
		m_lastEnvMap(nullptr)
	{
		initSubpixelJitterSamples();
	}
	PathTraceStage::~PathTraceStage()
	{

	}

	void PathTraceStage::initialize()
	{
		
		{
			RenderGraphNodeSlotDefinition slotdefsRtNode[] =
			{ {ResourceDimension::TEXTURE_2D,
				ResourceFormat::RGBA16_SFLOAT,
				RESOURCE_USAGE_STORAGE_TEXTURE,
				ACCESS_FLAGS_READ_WRITE,
				SHADERSTAGE_RT_RAYGENERATION,
				1,
				1} 
			};


			m_rtNode = getGraph()->createRayTraceNode(1, slotdefsRtNode, []
			(RenderGraphNode* node, const RenderGraphNodeExecutionContext& execContext, void* usrData)
				{
					static_cast<PathTraceStage*>(usrData)->executeRaytrace(execContext);
				},
				this, "RaytraceNode");
		}

		m_mergeStage.initialize(getRenderer(), getGraph());
		m_mergeStage.setInput(m_rtNode, 0);
		
		m_accStructureHelper.init(getRenderer());
	} 
	void PathTraceStage::shutdown()
	{
		if (m_raytracePso != YAPT_NULL_HANDLE)
		{
			Gfx::destroyRaytracePipelineState(getRenderer()->getGfxHandle(), m_raytracePso);
			m_raytracePso = YAPT_NULL_HANDLE;
		}
		 
		m_shaderTableHelper.deinit();
		m_accStructureHelper.deinit();

		m_mergeStage.shutdown();
	}
	 
	void PathTraceStage::onRenderResolutionChanged(const RenderResolutionDependantResourcesData& data)
	{

	
		m_resolveTargetWidth = data.newRenderResolutionWidth;
		m_resolveTargetHeight = data.newRenderResolutionHeight;

		//rt
		{
			RenderGraphResourceId rtTarget = m_rtNode->getRenderGraphResourceIdForSlot(0);
			const RenderGraphResourceDescription& desc = getGraph()->getRenderGraphResourceDescription(rtTarget);

			TextureDesc textureDesc(desc.resourceDimensions, desc.resourceFormat, desc.resourceUsage, m_resolveTargetWidth, m_resolveTargetHeight, 1, 1);
			TextureHandle rtTargetTex = data.resolutionDependantResourcesPool->requestTexture(textureDesc, "Main scene RT Color Target");
			getGraph()->setRenderGraphResourceTexture(rtTarget, rtTargetTex);
		}
		

		m_mergeStage.onRenderResolutionChanged(data, uvec2(m_resolveTargetWidth, m_resolveTargetHeight));

		clearAccumulatedFrames();
		updateEffectiveRaytraceResolution();
	}

	bool PathTraceStage::hasCameraMoved()
	{
		return getRenderer()->hasViewMoved();
	}
	bool PathTraceStage::hasSceneChanged()
	{
		bool objectsChanged =  getRenderer()->getMaterialManager().hasChanges() || getRenderer()->getMeshManager().hasChanges() || getRenderer()->getRenderObjectManager().hasChanges();
		bool skymapChanged = getRenderer()->getConcreteRendererConfiguration().getRendererVarValueInternal<RCPtr<Texture>>(RVARNAME_SKYBOX) != m_lastEnvMap;
		
		return objectsChanged || skymapChanged;
	}

	void PathTraceStage::onRenderGraphCompiled(const RenderGraphLifetimeData& data)
	{
		initRaytracePass(data);

		
		m_mergeStage.onRenderGraphCompiled(data);
	}

	void PathTraceStage::initRaytracePass(const RenderGraphLifetimeData& data)
	{
		ShaderLoader* loader = getRenderer()->getShaderLoader();
		const ShaderLoader::ShaderPipelineInfo* rayGen = loader->getShaderPipeline("rayGenPrimaryRays");
		const ShaderLoader::ShaderPipelineInfo* rayHit = loader->getShaderPipeline("rayHitDefault");
		const ShaderLoader::ShaderPipelineInfo* rayMiss = loader->getShaderPipeline("rayMissEnvironment");
		 
		ShaderPipelineReflection* reflArray[] = { rayGen->reflection, rayHit->reflection, rayMiss->reflection };
		m_rtLayout.initRaytraceLayoutFromShaderReflections(getRenderer()->getGfxHandle(), reflArray, countOf(reflArray));

		//texture and buffer array setup
		{
			BindlessTextureManager* texMngr = getRenderer()->getTextureManager();
			BindlessBufferManager* buffMngr = getRenderer()->getBufferManager();
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
					SamplerHandle samplerHandle = getRenderer()->getCoreResources()->getDefaultSampler(DefaultSamplerType::LINEAR_REPEAT);
					m_rtLayout.setStaticSamplers(nameMapping, &samplerHandle);
				}
			}
			
			{
				bool found = rayHit->reflection->getNameMapping(ShaderModuleType::LIBRARY_MODULE, "g_colorSampler", nameMapping);
				if (found)
				{
					SamplerHandle samplerHandle = getRenderer()->getCoreResources()->getDefaultSampler(DefaultSamplerType::NEAREST_REPEAT);
					m_rtLayout.setStaticSamplers(nameMapping, &samplerHandle);
				}
			}

			{
				bool found = rayHit->reflection->getNameMapping(ShaderModuleType::LIBRARY_MODULE, "g_pointSampler", nameMapping);
				if (found)
				{
					SamplerHandle samplerHandle = getRenderer()->getCoreResources()->getDefaultSampler(DefaultSamplerType::NEAREST_REPEAT);
					m_rtLayout.setStaticSamplers(nameMapping, &samplerHandle);
				}
			}
			
			{
				bool found = rayHit->reflection->getNameMapping(ShaderModuleType::LIBRARY_MODULE, "g_lutSampler", nameMapping);
				if (found)
				{
					SamplerHandle samplerHandle = getRenderer()->getCoreResources()->getDefaultSampler(DefaultSamplerType::LINEAR_CLAMP);
					m_rtLayout.setStaticSamplers(nameMapping, &samplerHandle);
				}
			}
			
			
		}
     
		m_rtLayout.compile();


		if (m_raytracePso != YAPT_NULL_HANDLE)
		{
			Gfx::destroyRaytracePipelineState(getRenderer()->getGfxHandle(), m_raytracePso);
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

		m_raytracePso = Gfx::createRaytracePipelineState(getRenderer()->getGfxHandle(), desc);

		m_shaderTableHelper.init(getRenderer(), m_raytracePso, desc.rayGenShaderTableConstantsSizeInBytes,
			desc.missShaderTableConstantsSizeInBytes, desc.hitGroupShaderTableConstantsSizeInBytes);


		m_rayTraceConstants.init(data.renderGraphLifetimeResources);
		m_randomSamples.init(data.renderGraphLifetimeResources);
		m_spectralDataConstants.init(data.renderGraphLifetimeResources);
	}

	bool  PathTraceStage::doesShaderTableNeedUpdate()
	{
		RenderObjectManager& roMngr = getRenderer()->getRenderObjectManager();
		if (m_shaderTableHelper.getShaderTable() == YAPT_NULL_HANDLE)
		{
			return true;
		}

		
		const RenderObjectId* ids;
		size_t idCount;

		roMngr.getCreatedEntries(ids, idCount);
		if (idCount > 0) return true;
		
		roMngr.getDestroyedEntries(ids, idCount);
		if (idCount > 0) return true;
		  
		roMngr.getModifiedEntries(ids, idCount);
		if (idCount > 0) return true;  
		
		RCPtr<Texture> skymap = getRenderer()->getConcreteRendererConfiguration().getRendererVarValueInternal<RCPtr<Texture>>(RVARNAME_SKYBOX);
		if (skymap.get() != m_lastEnvMap) return true;

		return false;

	}
	//stride and offset are assumed in dwords (uint32/float32) in the shader
	uvec2p PathTraceStage::packBufferInfo(uint32_t bufferIndex, uint32_t bufferStride, uint32_t bufferOffset)
	{
		glm::uvec2 v;
		v.x = bufferOffset;
		v.y = bufferStride << 16 | (bufferIndex & 0xFFFF);
		return v;
	}
	      
	void PathTraceStage::writeShaderTableEntryAndConstantData(size_t shaderTableIndex, const MaterialPerSubmeshArray& mat, const MeshInternal* mesh, size_t submeshIndex, ShaderTableEntry* entry)
	{
		auto getPackedBufferInfo = [](const MeshLayoutInfo& info, const AttributeMapping& attrMapping, const MeshInternal* mesh, const SubmeshDefinition& sm)
		{
			glm::uvec2 retVal;
			if (attrMapping.bufferIndex != uint32_t(-1))
			{

				const MeshBufferBinding&  bufferBinding = mesh->getVertexBuffer(attrMapping.bufferIndex);
				uint32_t bufferIndex = bufferBinding.buffer->getBindlessResourceArrayIndex();
				size_t vertexBufferOffset = bufferBinding.offsetInBytes;
				uint32_t stride = info.vertexBufferConfigurations[attrMapping.bufferIndex].stride;
				size_t offset = vertexBufferOffset + sm.vertexOffset * stride + info.vertexBufferConfigurations[attrMapping.bufferIndex].offsetFromVertexStart[attrMapping.attributeIndex];
				
				//make sure we can fit the data in packing
				assert(offset < 0xFFFFFFFF);
				assert(bufferIndex < 0xFFFF);
				assert(stride < 0xFFFF);
				 
				//from byteoffset to float/uint offset
				retVal = packBufferInfo(bufferIndex, stride / 4, (uint32_t)offset / 4);
			}
			else
			{
				retVal = glm::uvec2(-1, -1);
			}
			return retVal;
		};
		const SubmeshDefinition& sm = mesh->getSubmesh(submeshIndex);

		RayHitShaderTableConstantData rayHitConstants;
		//index buffer
		{
			const MeshBufferBinding& indexBufferBinding = mesh->getIndexBuffer();

			bool use16BitIndices = mesh->getIndexBufferFormat() == ResourceFormat::R16_UINT;
			size_t offsetInBytes = indexBufferBinding.offsetInBytes + sm.indexOffset * getFormatSizeInBytes(mesh->getIndexBufferFormat());
			uint8_t extraOffset = offsetInBytes % 4;
			uint8_t stride = use16BitIndices ? 2 : 3;
			rayHitConstants.indexBuffer = packBufferInfo(indexBufferBinding.buffer->getBindlessResourceArrayIndex(), (extraOffset << 8) | stride , uint32_t(offsetInBytes) / sizeof(uint32_t)); //16bit indices marked with stride of 2. If you change this, remember to change this assumptin in the shaders too!
		}
		//vertex data
		{
			const MeshLayoutInfo& info = mesh->getLayoutInfo();
			
			rayHitConstants.normalBuffer = getPackedBufferInfo(info, info.normal0, mesh, sm);
			rayHitConstants.tangentBuffer = getPackedBufferInfo(info, info.tangent0, mesh, sm);
			rayHitConstants.uvBuffer = getPackedBufferInfo(info, info.uv0, mesh, sm);
		}
		   
		//material data
		{
			const MaterialParameters& matParams = mat.getMaterialForSubmeshIndex(submeshIndex)->getMaterialParams();
			
			float transparency = matParams.transparency;
			float metalness = matParams.metalness;


			rayHitConstants.specAmountClearCoatAmountIORRoughness = vec4p(saturate(matParams.specularAmount),
				saturate(matParams.clearCoatAmount), matParams.clearCoatIOR, saturate(matParams.clearCoatRoughness));
			 
			rayHitConstants.albedoTransparency = vec4p(matParams.albedo, matParams.transparency);
			rayHitConstants.specularMetalness = vec4p(matParams.specular, matParams.metalness);
			rayHitConstants.absorptionDielectricIOR = vec4p(matParams.absorption, glm::clamp(matParams.dielectricIOR, 0.3f, 3.0f));
			rayHitConstants.emissiveRoughness = vec4p(matParams.emissive, saturate(matParams.roughness));
			rayHitConstants.anisotropy = saturate(matParams.anisotropy);
			rayHitConstants.anisotropyRotation = matParams.anisotropyRotation;

			rayHitConstants.materialMask = matParams.materialMask;
			rayHitConstants.thinFilmThickness = matParams.thinFilmThickness;

			rayHitConstants.cauchysCoefficients = matParams.cauchysCoeffs;
			rayHitConstants.sheenAmount = matParams.sheenAmount;
			rayHitConstants.sheenColorRoughness = vec4p(matParams.sheenTint, matParams.sheenRoughness);

			rayHitConstants.albedoTexIndexAndScale = uvec2p(matParams.albedoTex.textureIndex, matParams.albedoTex.packedScale);
			rayHitConstants.normalTexIndexAndScale = uvec2p(matParams.normalTex.textureIndex, matParams.normalTex.packedScale);
			rayHitConstants.ormTexIndexAndScale = uvec2p(matParams.ormTex.textureIndex, matParams.ormTex.packedScale);
			rayHitConstants.emissiveTexIndexAndScale = uvec2p(matParams.emissiveTex.textureIndex, matParams.emissiveTex.packedScale);
		}

		entry->extraDataInBytes = sizeof(RayHitShaderTableConstantData);
		memcpy(entry->shaderTableExtraData, &rayHitConstants, sizeof(RayHitShaderTableConstantData));
             
		entry->shaderIndexInPso = 0;
		entry->shaderTableIndex = shaderTableIndex;
	}
	 
	void PathTraceStage::updateShaderTable()
	{
		if (!doesShaderTableNeedUpdate()) return;
		
		RenderObjectManager& roMngr = getRenderer()->getRenderObjectManager();

		bool needToRecreateAllEntries = true;
		if (needToRecreateAllEntries)
		{
			const RenderObjectId* ids = roMngr.getAllIds();
			MaterialPerSubmeshArray* materials = roMngr.getAllMaterials();
			MeshInternal** meshes = roMngr.getAllMeshes();
			size_t entryCount = roMngr.getNumberOfObjects();

			m_shaderTableOffsetPerRenderObject.resize(entryCount);

			size_t shaderTableEntries = 0;
			for (size_t i = 0; i < entryCount; ++i)
			{
				m_shaderTableOffsetPerRenderObject[i] = shaderTableEntries;
				shaderTableEntries += meshes[i]->getSubmeshCount();
			}

			bool emptyHitGroup = shaderTableEntries == 0;

			m_shaderTableHelper.resize(1, 1, max((size_t)1, shaderTableEntries));

			for (size_t i = 0; i < entryCount; ++i)
			{
				size_t shaderTableOffset = m_shaderTableOffsetPerRenderObject[i];
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
				RCPtr<Texture> skymap = getRenderer()->getConcreteRendererConfiguration().getRendererVarValueInternal<RCPtr<Texture>>(RVARNAME_SKYBOX);
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

				m_lastEnvMap = skymap.get();
			}
			

		}
		else
		{
			//const RenderObjectId* ids;
			//size_t idCount;
		}

		m_shaderTableHelper.flush();
	}



	void PathTraceStage::prepare(const PrepareData& cntx)
	{

	}
	void PathTraceStage::update(const UpdateData& cntx)
	{
		if (hasSceneChanged() || hasCameraMoved())
		{
			clearAccumulatedFrames();
		}


		CombineSamplesSubStage::UpdateParams combineUpdate;
		combineUpdate.clearAccumulated = m_framesAccumulated == 0;
		combineUpdate.sourceTextureResolution = uvec2(m_raysPerFrameWidth, m_raysPerFrameHeight);
		combineUpdate.samplesPerPixel = getCurrentNumberOfSamplesPerPixel() + 1;
		combineUpdate.targetOffsetScaleBias = getCurrentResolveTargetTexelOffsetParams();
		m_mergeStage.update(combineUpdate);
		
		//update constants
		{
			float targetPixelWidth = 1.f / m_resolveTargetWidth;
			float targetPixelHeight = 1.f / m_resolveTargetHeight;

			RaytraceConstantData* rtConstants = m_rayTraceConstants.getData();

			//ray offset
			vec2p rayUVOffset(0.5 * targetPixelWidth, 0.5 * targetPixelHeight); //move to pixel center
			rayUVOffset += getCurrentRayGenerationOffset(); //move to (target) pixel being updated (will be 0 if doing fullres rt)

			//apply subpixel jitter
			if (m_applySubpixelJitter)
			{
				uint32_t subpixelJitterSampleIndex = getCurrentNumberOfSamplesPerPixel() % NUMBER_OF_SUBPIXEL_JITTER_SAMPLES;
				vec2p jitterSample = m_subpixelJitterSamples[subpixelJitterSampleIndex];
				rayUVOffset.x += jitterSample.x * targetPixelWidth;
				rayUVOffset.y += jitterSample.y * targetPixelHeight;
			}
			
			vec4 camPos(0.f, 0.f, 0.f, 1.f);
			mat4 viewToWorld = glm::inverse(getRenderer()->getCurrentRenderView().getView());
			camPos = viewToWorld * camPos;

			rtConstants->currentSampleIndex = uint32_t(getCurrentNumberOfSamplesPerPixel() % 0xFFFFFFFF);
			rtConstants->maxRayDepth = 16;
			rtConstants->rayUVOffset = rayUVOffset;
			rtConstants->cameraPosition = camPos;

			mat4 uvToViewTransform = glm::inverse(fromPlatformNDCToTextureSpace() * getRenderer()->getCurrentRenderView().getProjectionPlatform());

			rtConstants->uvToView = uvToViewTransform;
			rtConstants->viewToWorld = viewToWorld;

			//update samples
			updateSampledWavelengths();
			if ((getCurrentNumberOfSamplesPerPixel() % NUMBER_OF_RANDOM_SAMPLES) == 0)
			{
				if (m_framesAccumulated % (m_raysPerFrameDivisor * m_raysPerFrameDivisor) == 0)
				{
					updateSamples();
				}
			}
		}
		
		m_rayTraceConstants.flush();

		updateAccumulatedFrames();
		updateShaderTable();
	}
	
	 
	void PathTraceStage::executeRaytrace(const RenderGraphNodeExecutionContext& exec)
	{
		m_accStructureHelper.updateBottomLevelStructures(exec.cmdBuffer);

		auto assignPerInstanceParams = [this](size_t arrayIndex, RenderObjectId id, size_t submeshIndex, uint32_t& instanceIdOut, uint32_t& instanceMaskOut, size_t& hitGroupShaderTableOffset)
		{
			size_t shdTblOffset = m_shaderTableOffsetPerRenderObject[id];

			instanceIdOut = (uint32_t)(shdTblOffset + submeshIndex);
			instanceMaskOut = ~0;
			hitGroupShaderTableOffset = (shdTblOffset + submeshIndex);
		};
		 
		m_accStructureHelper.updateTopLevelStructures(exec.cmdBuffer, assignPerInstanceParams);
		//we will create the descset here since we need to acceleration structures done before this
		{
			if (m_rtDescSet != YAPT_NULL_HANDLE)
			{
				m_rtLayout.getDescriptorSetUtility(0).freeDescriptorSet(m_rtDescSet);
			}

			m_rtDescSet = m_rtLayout.getDescriptorSetUtility(0).getNewDescriptorSet();
			TextureViewHandle rtOutputUav = getGraph()->getTextureViewFromNodeSlot(m_rtNode->getSortedIndex(), 0);
			TopLevelAccelerationStructureHandle accStruct = m_accStructureHelper.getTopLevelAccelerationStructure();

			TextureViewHandle noiseTex = getRenderer()->getCoreResources()->getDefaultTextureView(DefaultTextureType::NOISE);

			TextureViewHandle singleScatterAlbedoNoFresnel = getRenderer()->getCoreResources()->getMultiScatteringLUTs().getSingleScatterDirectionalAlbedoNoFresnel();
			TextureViewHandle singleScatterAverageAlbedoNoFresnel = getRenderer()->getCoreResources()->getMultiScatteringLUTs().getSingleScatterAverageDirectionalAlbedoNoFresnel();
			TextureViewHandle singleAndMultiScatterAlbedo = getRenderer()->getCoreResources()->getMultiScatteringLUTs().getSingleAndMultiScatterDirectionalAlbedo();
			TextureViewHandle singleAndMultiScatterAverageAlbedo = getRenderer()->getCoreResources()->getMultiScatteringLUTs().getSingleAndMultiScatterAverageDirectionalAlbedo();
			TextureViewHandle singleScatterAlbedoTranslucentDenser = getRenderer()->getCoreResources()->getMultiScatteringLUTs().getSingleScatterDirectionalAlbedoTranslucentToDenser();
			TextureViewHandle singleScatterAverageAlbedoTranslucentLighter = getRenderer()->getCoreResources()->getMultiScatteringLUTs().getSingleScatterDirectionalAlbedoTranslucentToLighter();
							  
			TextureViewHandle singleScatterAvgAlbedoTranslucentDenser = getRenderer()->getCoreResources()->getMultiScatteringLUTs().getSingleScatterAverageAlbedoTranslucentToDenser();
			TextureViewHandle singleScatterAvgAverageAlbedoTranslucentLighter = getRenderer()->getCoreResources()->getMultiScatteringLUTs().getSingleScatterAverageAlbedoTranslucentToLighter();
							  
			TextureViewHandle sheenDirectionalAlbedo = getRenderer()->getCoreResources()->getMultiScatteringLUTs().getDirectionalAlbedoSheen();

			TextureViewHandle cieLUT = getRenderer()->getCoreResources()->getSpectralUtility().getXYZColorMatchingLUT();
			TextureViewHandle d65LUT = getRenderer()->getCoreResources()->getSpectralUtility().getD65IlluminantLUT();
			TextureViewHandle toSRGBLUT = getRenderer()->getCoreResources()->getSpectralUtility().getSRGBToSPDLUT();
			TextureViewHandle toREC2020LUT = getRenderer()->getCoreResources()->getSpectralUtility().getREC2020ToSPDLUT();
			

			DescriptorSetUpdate updates[] = {
				{1, 0, 1, DescriptorPtr(m_rayTraceConstants.getViewPtr())},
				{2, 0, 1, DescriptorPtr(m_randomSamples.getViewPtr())},
				{3, 0, 1, DescriptorPtr(m_spectralDataConstants.getViewPtr())},
				{4, 0, 1, DescriptorPtr(&accStruct) },
				{5, 0, 1, DescriptorPtr(&rtOutputUav)},
				{9, 0, 1, DescriptorPtr(&noiseTex)},
				{10, 0, 1, DescriptorPtr(&singleScatterAlbedoNoFresnel)},
				{11, 0, 1, DescriptorPtr(&singleScatterAverageAlbedoNoFresnel)},
				{12, 0, 1, DescriptorPtr(&singleAndMultiScatterAlbedo)},
				{13, 0, 1, DescriptorPtr(&singleAndMultiScatterAverageAlbedo)},
				{14, 0, 1, DescriptorPtr(&singleScatterAlbedoTranslucentDenser)},
				{15, 0, 1, DescriptorPtr(&singleScatterAverageAlbedoTranslucentLighter)},
				{16, 0, 1, DescriptorPtr(&singleScatterAvgAlbedoTranslucentDenser)},
				{17, 0, 1, DescriptorPtr(&singleScatterAvgAverageAlbedoTranslucentLighter)},
				{18, 0, 1, DescriptorPtr(&sheenDirectionalAlbedo)},
				{19, 0, 1, DescriptorPtr(&cieLUT)},
				{20, 0, 1, DescriptorPtr(&d65LUT)},
				{21, 0, 1, DescriptorPtr(&toREC2020LUT)},
				{22, 0, 1, DescriptorPtr(&toSRGBLUT)},
				
			};
			Gfx::updateDescriptorSet(getRenderer()->getGfxHandle(), m_rtDescSet, updates, countOf(updates));
		}
		 
		BindlessTextureManager* texMngr = getRenderer()->getTextureManager();
		BindlessBufferManager* buffMngr = getRenderer()->getBufferManager();
		GfxApiHandle gfx = getRenderer()->getGfxHandle();
		Gfx::setRaytracePipelineState(gfx, exec.cmdBuffer, m_raytracePso);

		DescriptorSetHandle bindings[] = { 
			 m_rtDescSet,
			 texMngr->getTextureArrayDescSet(),
			 buffMngr->getBufferArrayDescSet()
		};
		Gfx::bindDescriptorSets(gfx, exec.cmdBuffer, BindingPoint::BINDING_POINT_RAYTRACE, m_rtLayout.getPipelineLayoutHandle(), bindings, 0, countOf(bindings), nullptr, 0);

		Gfx::dispatchRays(gfx, exec.cmdBuffer, m_raysPerFrameWidth, m_raysPerFrameHeight, 1, m_shaderTableHelper.getShaderTable());
	}

	RenderStageConnection PathTraceStage::getOutputConnection(size_t id)
	{
		RenderStageConnection conn;
		switch (id)
		{
		case RAYTRACE_STAGE_CONNECTION_COLOR:
		{
			m_mergeStage.getOutput(&conn.node, conn.slot);
			break;
		}
		default:
			assert(false && "Stage connection id not recognized");
		}
		return conn;
	}
	void PathTraceStage::setInputConnection(size_t id, const RenderStageConnection& connection)
	{
		assert(false && "PathTraceStage accepts nothing as input!");
	}

	void PathTraceStage::clearAccumulatedFrames() 
	{ 
		m_framesAccumulated = 0;
	};

	void PathTraceStage::updateAccumulatedFrames()
	{
		++m_framesAccumulated;
	}
	
	void PathTraceStage::updateEffectiveRaytraceResolution()
	{
		m_raysPerFrameWidth = m_resolveTargetWidth / m_raysPerFrameDivisor;
		m_raysPerFrameHeight = m_resolveTargetHeight / m_raysPerFrameDivisor;
	}


	void PathTraceStage::setRayTraceResolutionReductionFactor(uint32_t factor)
	{
		assert(factor < 32);
		m_raysPerFrameDivisor = 1 << factor;

		assert(factor < m_resolveTargetWidth);
		assert(factor < m_resolveTargetHeight);

		clearAccumulatedFrames();
		updateEffectiveRaytraceResolution();
	}

	uvec4p PathTraceStage::getCurrentResolveTargetTexelOffsetParams()
	{    
		//scale & bias
		uint64_t frameIndex = m_framesAccumulated % (m_raysPerFrameDivisor * m_raysPerFrameDivisor);
		return glm::uvec4(m_raysPerFrameDivisor, m_raysPerFrameDivisor, frameIndex % m_raysPerFrameDivisor, frameIndex / m_raysPerFrameDivisor);
	}
	vec2p PathTraceStage::getCurrentRayGenerationOffset()
	{
		glm::uvec4 p = getCurrentResolveTargetTexelOffsetParams();
		glm::uvec2 pixelOffset(p.z, p.w);
		float targetPixelWidth = 1.f / m_resolveTargetWidth;
		float targetPixelHeight = 1.f / m_resolveTargetHeight;

		return vec2p(pixelOffset.x * targetPixelWidth, pixelOffset.y * targetPixelHeight);
	}

	uint64_t PathTraceStage::getCurrentNumberOfSamplesPerPixel()
	{
		uint32_t iterationsToFullResolution = m_raysPerFrameDivisor * m_raysPerFrameDivisor;
		uint64_t v = m_framesAccumulated / iterationsToFullResolution;
		return v;
	}
	
	void PathTraceStage::updateSamples()
	{       
		uint64_t sampleOffset = getCurrentNumberOfSamplesPerPixel();

		MathUtils::generateHaltonSequence(NUMBER_OF_RANDOM_SAMPLES, m_randomSamples.getData()->samples, sampleOffset);
		m_randomSamples.flush();
		

		
	}
	
	void PathTraceStage::updateSampledWavelengths()
	{
		uint64_t sampleOffset = getCurrentNumberOfSamplesPerPixel();
		SpectralDataConstants* data = m_spectralDataConstants.getData();
		if ((sampleOffset % SPECTRAL_SAMPLESET_COUNT) == 0)
		{
			std::array<float, SPECTRAL_SAMPLESET_COUNT * SPECTRAL_SAMPLES_COUNT> sampleLambdas;
			std::array<float, SPECTRAL_SAMPLESET_COUNT * SPECTRAL_SAMPLES_COUNT> samplePDFs;

			for (uint32_t i = 0; i < SPECTRAL_SAMPLESET_COUNT; ++i)
			{
				float rand = halton<float>(11, sampleOffset + i);
				SpectralUtility::generateSampleLambdas(rand, (size_t)SPECTRAL_SAMPLES_COUNT, sampleLambdas.data() + i * SPECTRAL_SAMPLES_COUNT, samplePDFs.data() + i * SPECTRAL_SAMPLES_COUNT, (float)SpectralUtility::getCIELUTMinLambda(), (float)SpectralUtility::getCIELUTMaxLambda());
			}
			
			memcpy(&data->spdSampleLambda, sampleLambdas.data(), sizeof(float) * sampleLambdas.size());
			memcpy(&data->spdSamplePdf, samplePDFs.data(), sizeof(float) * samplePDFs.size());

		}
		
		data->sampleSetOffset = sampleOffset % SPECTRAL_SAMPLESET_COUNT;
		m_spectralDataConstants.flush();
	}

	void PathTraceStage::initSubpixelJitterSamples()
	{
		MathUtils::generateHaltonSequence(NUMBER_OF_SUBPIXEL_JITTER_SAMPLES, m_subpixelJitterSamples, 0);
		//[0,1] -> [-0.5, 0.5], ie offsets from pixel center
		for (size_t i = 0; i < NUMBER_OF_SUBPIXEL_JITTER_SAMPLES; ++i)
		{
			m_subpixelJitterSamples[i] = (m_subpixelJitterSamples[i] - vec2p(0.5f, 0.5f));
		}

		
	}
	  
}