#include <Renderer/Shared/RenderPipeline/RaytraceStage.h>
#include <Renderer/Shared/RenderGraph/RenderGraph.h>
#include <Renderer/Shared/Utility/RenderResourcesPool.h>
#include <Renderer/Shared/Utility/PipelineStateDescriptionUtility.h>
#include <Renderer/Shared/CRenderer.h>
#include <Renderer/Shared/Utility/CoreRenderResourcesUtility.h>
#include <Renderer/Shared/Utility/RenderAPIAbstractionUtility.h>
#include <Renderer/Shared/BindlessTextureManager.h>
#include <Renderer/Shared/BindlessBufferManager.h>
#include <Math/RandUtility.h>
#include <Math/MathUtility.h>

#include <Renderer/Shared/TextureImpl.h>
#include <Renderer/Shared/BufferImpl.h>

#define MERGE_SAMPLES_WG_SIZE 8

using namespace YAPT::MathUtils;

namespace YAPT
{
	const size_t SHADERTABLE_EXTRA_GROW_AMOUNT = 100;
	RaytraceStage::RaytraceStage()
		:m_raytracePso(YAPT_NULL_HANDLE),
		m_rtDescSet(YAPT_NULL_HANDLE),
		m_resolveTargetWidth(0),
		m_resolveTargetHeight(0),
		m_raysPerFrameDivisor(1),
		m_applySubpixelJitter(true)
	{
		initSubpixelJitterSamples();
	}
	RaytraceStage::~RaytraceStage()
	{

	}

	void RaytraceStage::initialize()
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
					static_cast<RaytraceStage*>(usrData)->executeRaytrace(execContext);
				},
				this, "RaytraceNode");
		}

		{
			 

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

			m_mergeNode = getGraph()->createComputeNode(2, slotdefsMergeNode, []
			(RenderGraphNode* node, const RenderGraphNodeExecutionContext& execContext, void* usrData)
				{
					static_cast<RaytraceStage*>(usrData)->executeMergeToPrevious(execContext);
				},
				this, "MergeRtResultsNode");
		}
		 
		getGraph()->createEdge(m_rtNode, 0, m_mergeNode, 1);
		m_accStructureHelper.init(getRenderer());
	} 
	void RaytraceStage::shutdown()
	{
		if (m_raytracePso != YAPT_NULL_HANDLE)
		{
			Gfx::destroyRaytracePipelineState(getRenderer()->getGfxHandle(), m_raytracePso);
			m_raytracePso = YAPT_NULL_HANDLE;
		}
		 
		m_shaderTableHelper.deinit();

		m_clearMergeBufferPass.deinit();
		m_mergePass.deinit();
		m_accStructureHelper.deinit();
	}
	 
	void RaytraceStage::onRenderResolutionChanged(const RenderResolutionDependantResourcesData& data)
	{

	
		m_resolveTargetWidth = data.newRenderResolutionWidth;
		m_resolveTargetHeight = data.newRenderResolutionHeight;

		//rt
		{
			RenderGraphResourceId rtTarget = m_rtNode->getRenderGraphResourceIdForSlot(0);
			const RenderGraphResourceDescription& desc = getGraph()->getRenderGraphResourceDescription(rtTarget);

			TextureDesc textureDesc(desc.resourceDimensions, desc.resourceFormat, desc.resourceUsage, m_resolveTargetWidth, m_resolveTargetHeight, 1, 1);
			ResourceStateDescription state;

			state = getGraph()->getLastStateForResource(rtTarget);
			TextureHandle rtTargetTex = data.resolutionDependantResourcesPool->requestTexture(textureDesc, state, "Main scene RT Color Target");

			m_bindingsUtility.setDeferredRenderGraphResourceBinding(rtTarget, rtTargetTex, state);
		}
		

		//"merge"
		{

			setupMergePass(data.resolutionDependantResourcesPool);
			RenderGraphResourceId resId = m_mergeNode->getRenderGraphResourceIdForSlot(0);


			const RenderGraphResourceDescription& desc = getGraph()->getRenderGraphResourceDescription(resId);
			TextureDesc textureDesc(desc.resourceDimensions, desc.resourceFormat, desc.resourceUsage, m_resolveTargetWidth, m_resolveTargetHeight, 1, 1);

			ResourceStateDescription state;

			state = getGraph()->getLastStateForResource(resId);
			TextureHandle tex = data.resolutionDependantResourcesPool->requestTexture(textureDesc, state, "merged RT result");

			m_bindingsUtility.setDeferredRenderGraphResourceBinding(resId, tex, state);
		}

		clearAccumulatedFrames();
		updateEffectiveRaytraceResolution();
	}

	bool RaytraceStage::hasCameraMoved()
	{
		return getRenderer()->hasViewMoved();
	}
	bool RaytraceStage::hasSceneChanged()
	{
		return getRenderer()->getMaterialManager().hasChanges() || getRenderer()->getMeshManager().hasChanges() || getRenderer()->getRenderObjectManager().hasChanges();
	}

	void RaytraceStage::onRenderGraphCompiled(const RenderGraphLifetimeData& data)
	{
		initRaytracePass(data);

		//mergepass init (properly setup later)
		ShaderLoader* loader = getRenderer()->getShaderLoader();
		const ShaderLoader::ShaderPipelineInfo* merge = loader->getShaderPipeline("mergeRaytraceResults");

		StaticSamplerEntry samplers[] = { {"colorSampler", getRenderer()->getCoreResources()->getDefaultSampler(DefaultSamplerType::LINEAR_REPEAT)} };
		m_mergePass.init(getRenderer(), merge, samplers, countOf(samplers));
		m_mergeSamplesConstants.init(data.renderGraphLifetimeResources);

		const ShaderLoader::ShaderPipelineInfo* clearMergeBuffer = loader->getShaderPipeline("clearRaytraceMergeTarget");
		m_clearMergeBufferPass.init(getRenderer(), clearMergeBuffer, nullptr, 0);
		m_clearMergeBufferConstants.init(data.renderGraphLifetimeResources);
		
	}

	void RaytraceStage::initRaytracePass(const RenderGraphLifetimeData& data)
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
		{
			ShaderPipelineReflection::NameMapping nameMapping;
			bool found = rayMiss->reflection->getNameMapping(ShaderModuleType::LIBRARY_MODULE, "g_colorSampler", nameMapping);
  
			if (found)
			{
				SamplerHandle samplerHandle = getRenderer()->getCoreResources()->getDefaultSampler(DefaultSamplerType::LINEAR_REPEAT);
				m_rtLayout.setStaticSamplers(nameMapping, &samplerHandle);
			}
			     
			found = rayHit->reflection->getNameMapping(ShaderModuleType::LIBRARY_MODULE, "g_pointSampler", nameMapping);
			if (found)
			{
				SamplerHandle samplerHandle = getRenderer()->getCoreResources()->getDefaultSampler(DefaultSamplerType::NEAREST_REPEAT);
				m_rtLayout.setStaticSamplers(nameMapping, &samplerHandle);
			}
			 
			found = rayHit->reflection->getNameMapping(ShaderModuleType::LIBRARY_MODULE, "g_lutSampler", nameMapping);
			if(found)
			{
				SamplerHandle samplerHandle = getRenderer()->getCoreResources()->getDefaultSampler(DefaultSamplerType::LINEAR_CLAMP);
				m_rtLayout.setStaticSamplers(nameMapping, &samplerHandle);
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
	}

	bool  RaytraceStage::doesShaderTableNeedUpdate()
	{
		if (m_shaderTableHelper.getShaderTable() == YAPT_NULL_HANDLE) return true;
		RenderObjectManager& roMngr = getRenderer()->getRenderObjectManager();
		const RenderObjectId* ids;
		size_t idCount;

		roMngr.getCreatedEntries(ids, idCount);
		if (idCount > 0) return true;
		
		roMngr.getDestroyedEntries(ids, idCount);
		if (idCount > 0) return true;
		  
		roMngr.getModifiedEntries(ids, idCount);
		if (idCount > 0) return true;  
		return false;
	}
	//stride and offset are assumed in dwords (uint32/float32) in the shader
	uvec2p RaytraceStage::packBufferInfo(uint32_t bufferIndex, uint32_t bufferStride, uint32_t bufferOffset)
	{
		glm::uvec2 v;
		v.x = bufferOffset;
		v.y = bufferStride << 16 | (bufferIndex & 0xFFFF);
		return v;
	}
	      
	void RaytraceStage::writeShaderTableEntryAndConstantData(RenderObjectId id, const MaterialInternal* mat, const MeshInternal* mesh, ShaderTableEntry* entry)
	{
		auto getPackedBufferInfo = [](const MeshLayoutInfo& info, const AttributeMapping& attrMapping, const MeshInternal* mesh)
		{
			glm::uvec2 retVal;
			if (attrMapping.bufferIndex != uint32_t(-1))
			{
				const MeshBufferBinding&  bufferBinding = mesh->getVertexBuffer(attrMapping.bufferIndex);
				uint32_t bufferIndex = bufferBinding.buffer->getBindlessResourceArrayIndex();
				size_t vertexBufferOffset = bufferBinding.offsetInBytes;
				uint32_t stride = info.vertexBufferConfigurations[attrMapping.bufferIndex].stride;
				size_t offset = vertexBufferOffset + info.vertexBufferConfigurations[attrMapping.bufferIndex].offsetFromVertexStart[attrMapping.attributeIndex];
				
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
		RayHitShaderTableConstantData rayHitConstants;
		//index buffer
		{
			const MeshBufferBinding& indexBufferBinding = mesh->getIndexBuffer();
			size_t offsetInBytes = indexBufferBinding.offsetInBytes;
			rayHitConstants.indexBuffer = packBufferInfo(indexBufferBinding.buffer->getBindlessResourceArrayIndex(), 3, uint32_t(offsetInBytes) / sizeof(uint32_t));
		}
		//vertex data
		{
			const MeshLayoutInfo& info = mesh->getLayoutInfo();

			rayHitConstants.normalBuffer = getPackedBufferInfo(info, info.normal0, mesh);
			rayHitConstants.tangentBuffer = getPackedBufferInfo(info, info.tangent0, mesh);
			rayHitConstants.uvBuffer = getPackedBufferInfo(info, info.uv0, mesh);
		}
		   
		//material data
		{
			const MaterialParameters& matParams = mat->getMaterialParams();
			
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

			rayHitConstants.sheenAmount = matParams.sheenAmount;
			rayHitConstants.sheenColorRoughness = vec4p(matParams.sheenTint, matParams.sheenRoughness);

			rayHitConstants.albedoTexIndex = matParams.albedoTexIndex;
			rayHitConstants.normalTexIndex = matParams.normalTexIndex;
			rayHitConstants.ormTexIndex = matParams.ormTexIndex;
			rayHitConstants.emissiveTexIndex = matParams.emissiveTexIndex;
		}

		entry->extraDataInBytes = sizeof(RayHitShaderTableConstantData);
		memcpy(entry->shaderTableExtraData, &rayHitConstants, sizeof(RayHitShaderTableConstantData));
             
		entry->shaderIndexInPso = 0;
		entry->shaderTableIndex = id;
	}
	 
	void RaytraceStage::updateShaderTable()
	{
		if (!doesShaderTableNeedUpdate()) return;
		
		RenderObjectManager& roMngr = getRenderer()->getRenderObjectManager();

		bool needToRecreateAllEntries = true;

		if (roMngr.getHighestId() >= m_shaderTableHelper.getNumberOfHitGroupEntries() )
		{
			m_shaderTableHelper.resize(1, 1, roMngr.getNumberOfObjects() + SHADERTABLE_EXTRA_GROW_AMOUNT);
			needToRecreateAllEntries = true;
		}
		 
		if (needToRecreateAllEntries)
		{
			const RenderObjectId* ids = roMngr.getAllIds();
			MaterialInternal** materials = roMngr.getAllMaterials();
			MeshInternal** meshes = roMngr.getAllMeshes();
			size_t entryCount = roMngr.getNumberOfObjects();

			for (size_t i = 0; i < entryCount; ++i)
			{
				ShaderTableEntry* entry = m_shaderTableHelper.appendHitGroupUpdate();

				writeShaderTableEntryAndConstantData(ids[i], materials[i], meshes[i], entry);
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
				missConstantData.cubeMapIndex = static_cast<TextureImpl*>(skymap.get())->getBindlessResourceArrayIndex();

				ShaderTableEntry* entry = m_shaderTableHelper.appendMissShaderUpdate();
				entry->shaderIndexInPso = 0;
				entry->shaderTableIndex = 0;
				entry->extraDataInBytes = sizeof(RayMissShaderTableConstantData);
				memcpy(entry->shaderTableExtraData, &missConstantData, sizeof(RayMissShaderTableConstantData));
			}
			

		}
		else
		{
			//const RenderObjectId* ids;
			//size_t idCount;
		}

		m_shaderTableHelper.flush();
	}


	void RaytraceStage::setupMergePass(RenderResourcesPool* pool)
	{
		m_mergePass.createPipelineState();
		m_clearMergeBufferPass.createPipelineState();
	}

	void RaytraceStage::prepare(const PrepareData& cntx)
	{
		m_bindingsUtility.flushDeferredRenderGraphResourceBindings(getGraph());
	}
	void RaytraceStage::update(const UpdateData& cntx)
	{
		if (hasSceneChanged() || hasCameraMoved())
		{
			clearAccumulatedFrames();
		}

		//descset shenanigans
		bool haveTexturesChanged = getGraph()->isResourceBoundThisFrame(m_mergeNode->getRenderGraphResourceIdForSlot(0));
		haveTexturesChanged = haveTexturesChanged || getGraph()->isResourceBoundThisFrame(m_mergeNode->getRenderGraphResourceIdForSlot(1));
		if (haveTexturesChanged)
		{
			//clear accumulation buffer
			{
				TextureViewHandle mergeTarget = getGraph()->getTextureViewFromNodeSlot(m_mergeNode->getSortedIndex(), 0);

				DescriptorSetUpdate updates[] = {
					{0, 0, 1, nullptr, m_clearMergeBufferConstants.getViewPtr(), nullptr},
					{1, 0, 1, &mergeTarget, nullptr, nullptr},
				};
				m_clearMergeBufferPass.updateDescriptorSet(0, updates, countOf(updates));
			}

			//Merge
			{
				TextureViewHandle mergeTarget = getGraph()->getTextureViewFromNodeSlot(m_mergeNode->getSortedIndex(), 0);
				TextureViewHandle mergeSource = getGraph()->getTextureViewFromNodeSlot(m_mergeNode->getSortedIndex(), 1);

				DescriptorSetUpdate updates[] = {
					{0, 0, 1, nullptr, m_mergeSamplesConstants.getViewPtr(), nullptr},
					{1, 0, 1, &mergeSource, nullptr, nullptr},
					{2, 0, 1, &mergeTarget, nullptr, nullptr}
				};
				m_mergePass.updateDescriptorSet(0, updates, countOf(updates));
			}
			
		}
		//update constants
		{
			float targetPixelWidth = 1.f / m_resolveTargetWidth;
			float targetPixelHeight = 1.f / m_resolveTargetHeight;


			if (m_framesAccumulated == 0)
			{
				ClearAccumulatedSamplesParams* params = m_clearMergeBufferConstants.getData();
				params->targetTextureDimensions = glm::uvec4(m_resolveTargetWidth, m_resolveTargetHeight, 0, 0);
				params->clearValue = vec4p(0.0, 0.f, 0.f, 0.f);
				m_clearMergeBufferConstants.flush();
			}

			MergeNewSamplesParams* mergeSamplesParams = m_mergeSamplesConstants.getData();
			mergeSamplesParams->sourceTextureDimensions = glm::uvec2(m_raysPerFrameWidth, m_raysPerFrameHeight);
			mergeSamplesParams->targetTextureOffsetScaleBias = getCurrentResolveTargetTexelOffsetParams();

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

			rtConstants->currentSampleIndex = getCurrentNumberOfSamplesPerPixel();
			rtConstants->maxRayDepth = 16;
			rtConstants->rayUVOffset = rayUVOffset;
			rtConstants->cameraPosition = camPos;

			mat4 uvToViewTransform = glm::inverse(fromPlatformNDCToTextureSpace() * getRenderer()->getCurrentRenderView().getProjectionPlatform());

			rtConstants->uvToView = uvToViewTransform;
			rtConstants->viewToWorld = viewToWorld;

			//update samples

			if ((getCurrentNumberOfSamplesPerPixel() % NUMBER_OF_RANDOM_SAMPLES) == 0)
			{
				if (m_framesAccumulated % (m_raysPerFrameDivisor * m_raysPerFrameDivisor) == 0)
				{
					updateRandomSamples();
				}
			}
		}
		
		m_mergeSamplesConstants.flush();
		m_rayTraceConstants.flush();

		updateAccumulatedFrames();
		updateShaderTable();
	}
	
	 
	void RaytraceStage::executeRaytrace(const RenderGraphNodeExecutionContext& exec)
	{
		m_accStructureHelper.updateBottomLevelStructures(exec.cmdBuffer);



		auto assignPerInstanceParams = [](size_t arrayIndex, RenderObjectId id, uint32_t& instanceIdOut, uint32_t& instanceMaskOut, size_t& hitGroupShaderTableOffset)
		{
			instanceIdOut = (uint32_t)id;
			instanceMaskOut = ~0;
			hitGroupShaderTableOffset = id;
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
			BufferViewHandle accStructView = Gfx::getBufferView(getRenderer()->getGfxHandle(), m_accStructureHelper.getTopLevelAccelerationStructure());

			TextureViewHandle singleScatterAlbedoNoFresnel = getRenderer()->getCoreResources()->getMultiScatteringLUTs().getSingleScatterDirectionalAlbedoNoFresnel();
			TextureViewHandle singleScatterAverageAlbedoNoFresnel = getRenderer()->getCoreResources()->getMultiScatteringLUTs().getSingleScatterAverageDirectionalAlbedoNoFresnel();
			TextureViewHandle singleAndMultiScatterAlbedo = getRenderer()->getCoreResources()->getMultiScatteringLUTs().getSingleAndMultiScatterDirectionalAlbedo();
			TextureViewHandle singleAndMultiScatterAverageAlbedo = getRenderer()->getCoreResources()->getMultiScatteringLUTs().getSingleAndMultiScatterAverageDirectionalAlbedo();
			TextureViewHandle singleScatterAlbedoTranslucentDenser = getRenderer()->getCoreResources()->getMultiScatteringLUTs().getSingleScatterDirectionalAlbedoTranslucentToDenser();
			TextureViewHandle singleScatterAverageAlbedoTranslucentLighter = getRenderer()->getCoreResources()->getMultiScatteringLUTs().getSingleScatterDirectionalAlbedoTranslucentToLighter();
							  
			TextureViewHandle singleScatterAvgAlbedoTranslucentDenser = getRenderer()->getCoreResources()->getMultiScatteringLUTs().getSingleScatterAverageAlbedoTranslucentToDenser();
			TextureViewHandle singleScatterAvgAverageAlbedoTranslucentLighter = getRenderer()->getCoreResources()->getMultiScatteringLUTs().getSingleScatterAverageAlbedoTranslucentToLighter();
							  
			TextureViewHandle sheenDirectionalAlbedo = getRenderer()->getCoreResources()->getMultiScatteringLUTs().getDirectionalAlbedoSheen();

			TextureViewHandle noiseTex = getRenderer()->getCoreResources()->getDefaultTextureView(DefaultTextureType::NOISE);
			

			DescriptorSetUpdate updates[] = {
				{1, 0, 1, nullptr, m_rayTraceConstants.getViewPtr(), nullptr},
				{2, 0, 1, nullptr, m_randomSamples.getViewPtr(), nullptr},
				{3, 0, 1, nullptr, &accStructView, nullptr },
				{4, 0, 1, &rtOutputUav, nullptr, nullptr},
				{10, 0, 1, &singleScatterAlbedoNoFresnel, nullptr, nullptr},
				{11, 0, 1, &singleScatterAverageAlbedoNoFresnel, nullptr, nullptr},
				{12, 0, 1, &singleAndMultiScatterAlbedo, nullptr, nullptr},
				{13, 0, 1, &singleAndMultiScatterAverageAlbedo, nullptr, nullptr},
				{14, 0, 1, &singleScatterAlbedoTranslucentDenser, nullptr, nullptr},
				{15, 0, 1, &singleScatterAverageAlbedoTranslucentLighter, nullptr, nullptr},
				{16, 0, 1, &singleScatterAvgAlbedoTranslucentDenser, nullptr, nullptr},
				{17, 0, 1, &singleScatterAvgAverageAlbedoTranslucentLighter, nullptr, nullptr},
				{18, 0, 1, &sheenDirectionalAlbedo, nullptr, nullptr},
				{20, 0, 1, &noiseTex, nullptr, nullptr}
			};
			Gfx::updateDescriptorSet(getRenderer()->getGfxHandle(), m_rtDescSet, updates, countOf(updates));
		}
		 
		BindlessTextureManager* texMngr = getRenderer()->getTextureManager();
		BindlessBufferManager* buffMngr = getRenderer()->getBufferManager();
		GfxApiHandle gfx = getRenderer()->getGfxHandle();
		Gfx::setRaytracePipelineState(gfx, exec.cmdBuffer, m_raytracePso);

		DescriptorSetBinding bindings[] = { 
			{0, m_rtDescSet},
			{1, texMngr->getTextureArrayDescSet() },
			{2, buffMngr->getBufferArrayDescSet() }
		};
		Gfx::bindDescriptorSets(gfx, exec.cmdBuffer, bindings, countOf(bindings), nullptr, 0);

		Gfx::dispatchRays(gfx, exec.cmdBuffer, m_raysPerFrameWidth, m_raysPerFrameHeight, 1, m_shaderTableHelper.getShaderTable());
	}

	void RaytraceStage::executeMergeToPrevious(const RenderGraphNodeExecutionContext& exec)
	{
		if (m_framesAccumulated == 1)
		{
			uint32_t dispatchX = (m_resolveTargetWidth + MERGE_SAMPLES_WG_SIZE - 1) / MERGE_SAMPLES_WG_SIZE;
			uint32_t dispatchY = (m_resolveTargetHeight + MERGE_SAMPLES_WG_SIZE - 1) / MERGE_SAMPLES_WG_SIZE;

			m_clearMergeBufferPass.dispatch(exec.cmdBuffer, dispatchX, dispatchY, 1);
		}


		uint32_t rtWidth = m_raysPerFrameWidth;
		uint32_t rtHeight = m_raysPerFrameHeight;

		uint32_t dispatchX = (rtWidth + MERGE_SAMPLES_WG_SIZE - 1) / MERGE_SAMPLES_WG_SIZE;
		uint32_t dispatchY = (rtHeight + MERGE_SAMPLES_WG_SIZE - 1) / MERGE_SAMPLES_WG_SIZE;

		m_mergePass.dispatch(exec.cmdBuffer, dispatchX, dispatchY, 1);
	}

	RenderStageConnection RaytraceStage::getOutputConnection(size_t id)
	{
		RenderStageConnection conn;
		switch (id)
		{
		case RAYTRACE_STAGE_CONNECTION_COLOR:
		{
			conn.node = m_mergeNode;
			conn.slot = 0;
			break;
		}
		default:
			assert(false && "Stage connection id not recognized");
		}
		return conn;
	}
	void RaytraceStage::setInputConnection(size_t id, const RenderStageConnection& connection)
	{
		assert(false && "RaytraceStage accepts nothing as input!");
	}

	void RaytraceStage::clearAccumulatedFrames() 
	{ 
		m_framesAccumulated = 0;
	};

	void RaytraceStage::updateAccumulatedFrames()
	{
		++m_framesAccumulated;
	}
	
	void RaytraceStage::updateEffectiveRaytraceResolution()
	{
		m_raysPerFrameWidth = m_resolveTargetWidth / m_raysPerFrameDivisor;
		m_raysPerFrameHeight = m_resolveTargetHeight / m_raysPerFrameDivisor;
	}


	void RaytraceStage::setRayTraceResolutionReductionFactor(uint32_t factor)
	{
		assert(factor < 32);
		m_raysPerFrameDivisor = 1 << factor;

		assert(factor < m_resolveTargetWidth);
		assert(factor < m_resolveTargetHeight);

		clearAccumulatedFrames();
		updateEffectiveRaytraceResolution();
	}

	uvec4p RaytraceStage::getCurrentResolveTargetTexelOffsetParams()
	{    
		//scale & bias
		uint32_t frameIndex = m_framesAccumulated % (m_raysPerFrameDivisor * m_raysPerFrameDivisor);
		return glm::uvec4(m_raysPerFrameDivisor, m_raysPerFrameDivisor, frameIndex % m_raysPerFrameDivisor, frameIndex / m_raysPerFrameDivisor);
	}
	vec2p RaytraceStage::getCurrentRayGenerationOffset()
	{
		glm::uvec4 p = getCurrentResolveTargetTexelOffsetParams();
		glm::uvec2 pixelOffset(p.z, p.w);
		float targetPixelWidth = 1.f / m_resolveTargetWidth;
		float targetPixelHeight = 1.f / m_resolveTargetHeight;

		return vec2p(pixelOffset.x * targetPixelWidth, pixelOffset.y * targetPixelHeight);
	}

	uint32_t RaytraceStage::getCurrentNumberOfSamplesPerPixel()
	{
		uint32_t iterationsToFullResolution = m_raysPerFrameDivisor * m_raysPerFrameDivisor;
		uint32_t v = m_framesAccumulated / iterationsToFullResolution;
		return v;
	}
	
	void RaytraceStage::updateRandomSamples()
	{       
		MathUtils::generateHaltonSequence(NUMBER_OF_RANDOM_SAMPLES, m_randomSamples.getData()->samples, getCurrentNumberOfSamplesPerPixel());
		m_randomSamples.flush();
	}
	
	void RaytraceStage::initSubpixelJitterSamples()
	{
		MathUtils::generateHaltonSequence(NUMBER_OF_SUBPIXEL_JITTER_SAMPLES, m_subpixelJitterSamples, 0);
		//[0,1] -> [-0.5, 0.5], ie offsets from pixel center
		for (size_t i = 0; i < NUMBER_OF_SUBPIXEL_JITTER_SAMPLES; ++i)
		{
			m_subpixelJitterSamples[i] = (m_subpixelJitterSamples[i] - vec2p(0.5f, 0.5f));
		}

		
	}
	  
}