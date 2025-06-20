#include <Renderer/Shared/RenderPipeline/PathTracer/BackwardsPathIntegratorSubStage.h>
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
	

	BackwardsPathIntegratorSubStage::BackwardsPathIntegratorSubStage(bool useNextEventEstimation)
		:m_raytracePso(YAPT_NULL_HANDLE),
		m_rtCommonResourcesDescSet(YAPT_NULL_HANDLE),
		m_rtMiscResourcesDescSet(YAPT_NULL_HANDLE),
		m_useNEE(useNextEventEstimation)
		
	{

	}
	BackwardsPathIntegratorSubStage::~BackwardsPathIntegratorSubStage()
	{

	}

	void BackwardsPathIntegratorSubStage::initialize(AccelerationStructureProvider* accStructProvider, CRenderer* rend, RenderGraph* graph, BindlessMaterialManager* matMngr, BindlessMeshManager* meshMngr)
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
				static_cast<BackwardsPathIntegratorSubStage*>(usrData)->executeRaytrace(execContext);
			},
			this, "RaytraceNode");
	}
	void BackwardsPathIntegratorSubStage::shutdown()
	{
		m_raytraceCommon.shutdown();
		if (m_raytracePso != YAPT_NULL_HANDLE)
		{
			Gfx::destroyRaytracePipelineState(m_renderer->getGfxHandle(), m_raytracePso);
			m_raytracePso = YAPT_NULL_HANDLE;
		}

		m_shaderTableHelper.deinit();
	}
	void BackwardsPathIntegratorSubStage::onRenderGraphCompiled(const RenderStage::RenderGraphLifetimeData& data)
	{
		ShaderLoader* loader = m_renderer->getShaderLoader();
		const ShaderLoader::ShaderPipelineInfo* rayGen = loader->getShaderPipeline("rayGenPrimaryRays");
		const ShaderLoader::ShaderPipelineInfo* rayHit = loader->getShaderPipeline(m_useNEE ? "rayHitDefaultWithNEE" : "rayHitDefault");
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



		{
			m_raytraceCommon.setupCommonSamplers(m_renderer, m_rtLayout, *rayMiss->reflection, ShaderModuleType::LIBRARY_MODULE);
			m_raytraceCommon.setupCommonSamplers(m_renderer, m_rtLayout, *rayHit->reflection, ShaderModuleType::LIBRARY_MODULE);
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

		m_raytraceCommon.initialize(m_renderer, data.renderGraphLifetimeResources, m_materialMngr, m_meshMngr);

	}
	void BackwardsPathIntegratorSubStage::onRenderResolutionChanged(const RenderStage::RenderResolutionDependantResourcesData& data, uvec2 newResolution)
	{
		m_renderResolution = newResolution;

		RenderGraphResourceId rtTarget = m_rtNode->getRenderGraphResourceIdForSlot(0);
		const RenderGraphResourceDescription& desc = m_graph->getRenderGraphResourceDescription(rtTarget);

		TextureDesc textureDesc(desc.resourceDimensions, desc.resourceFormat, desc.resourceUsage, newResolution.x, newResolution.y, 1, 1);
		TextureHandle rtTargetTex = data.resolutionDependantResourcesPool->requestTexture(textureDesc, "Main scene RT Color Target");
		m_graph->setRenderGraphResourceTexture(rtTarget, rtTargetTex);
		
	}

	void BackwardsPathIntegratorSubStage::prepare(const RenderStage::PrepareData& params)
	{
		m_accStructProvider->prepareAccelerationStructure();

		RaytraceCommonResources::PrepareParams p;
		p.prepareTasksPool = params.prepareTasksPool;
		m_raytraceCommon.prepare(p);
	}

	void BackwardsPathIntegratorSubStage::update(const BackwardsPathIntegratorSubStage::UpdateParams& params)
	{
		RaytraceCommonResources::UpdateParams p;
		p.rayGenOffsetInTexels = params.rayGenOffsetInTexels;
		p.raysPerFrame = params.raysPerFrame;
		p.renderResolution = m_renderResolution;
		p.sampleOffset = params.sampleOffset;
		p.spectralSampleOffset = params.sampleOffset;
		p.updateTasksPool = params.stageUpdateContext->updateTasksPool;
		m_raytraceCommon.update(p);
		
		m_lastUpdateParams = params;

	}

	void BackwardsPathIntegratorSubStage::getOutput(RenderGraphNode** node, size_t& slotOut)
	{
		*node = m_rtNode;
		slotOut = 0;
	}

	void BackwardsPathIntegratorSubStage::writeShaderTableEntryAndConstantData(size_t shaderTableIndex, const MaterialPerSubmeshArray& mat, MeshIndex meshID, size_t submeshIndex, ShaderTableEntry* entry)
	{

		size_t matId = mat.getMaterialIDForSubmeshIndex(submeshIndex);
		size_t meshId = meshID;

		size_t matIndex = m_materialMngr->getEntryIndexForMaterialId(matId);
		size_t meshIndex = m_meshMngr->getEntryIndexForMeshIdAndSubmesh(meshId, submeshIndex);

		RayHitShaderTableConstantData rayHitConstants;
		rayHitConstants.materialAndMeshIndices = uvec2p(matIndex, meshIndex);


		entry->extraDataInBytes = sizeof(RayHitShaderTableConstantData);
		memcpy(entry->shaderTableExtraData, &rayHitConstants, sizeof(RayHitShaderTableConstantData));

		entry->shaderIndexInPso = 0;
		entry->shaderTableIndex = shaderTableIndex;
	}

	void BackwardsPathIntegratorSubStage::sceneChanged(const RenderObjectId* ids, MaterialPerSubmeshArray* materials, MeshIndex* meshes, size_t* instanceOffsets, size_t objectCount, size_t instancesCount)
	{
		m_raytraceCommon.sceneChanged(ids, materials, meshes, instanceOffsets, objectCount, instancesCount);

		bool emptyHitGroup = instancesCount == 0;

		m_shaderTableHelper.resize(1, 1, max((size_t)1, instancesCount));
		MeshManager& meshMngr = m_renderer->getMeshManager();

		for (size_t i = 0; i < objectCount; ++i)
		{
			MeshInternal* mesh = meshMngr.getMeshInternal(meshes[i]);
			size_t shaderTableOffset = instanceOffsets[i];
			for (size_t k = 0; k < mesh->getSubmeshCount(); ++k)
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



	void BackwardsPathIntegratorSubStage::executeRaytrace(const RenderGraphNodeExecutionContext& exec)
	{
		//we will create the descset here since we need to acceleration structures done before this
		{
			if (m_rtCommonResourcesDescSet != YAPT_NULL_HANDLE)
			{
				m_rtLayout.getDescriptorSetUtility(0).freeDescriptorSet(m_rtCommonResourcesDescSet);
			}
			m_rtCommonResourcesDescSet = m_rtLayout.getDescriptorSetUtility(0).getNewDescriptorSet();

			if (m_rtMiscResourcesDescSet != YAPT_NULL_HANDLE)
			{
				m_rtLayout.getDescriptorSetUtility(3).freeDescriptorSet(m_rtMiscResourcesDescSet);
			}
			m_rtMiscResourcesDescSet = m_rtLayout.getDescriptorSetUtility(3).getNewDescriptorSet();

			m_raytraceCommon.updateCommonResourcesToDescriptorSet(m_rtCommonResourcesDescSet);

			TextureViewHandle rtOutputUav = m_graph->getTextureViewFromNodeSlot(m_rtNode->getSortedIndex(), 0);
			TopLevelAccelerationStructureHandle accStruct = m_accStructProvider->getAccelerationStructure();

			DescriptorSetUpdate updates[] = {
			{0, 0, 1, DescriptorPtr(&accStruct)},
			{1, 0, 1, DescriptorPtr(&rtOutputUav)},
			};

			Gfx::updateDescriptorSet(m_renderer->getGfxHandle(), m_rtMiscResourcesDescSet, updates, countOf(updates));
		}

		BindlessTextureManager* texMngr = m_renderer->getTextureManager();
		BindlessBufferManager* buffMngr = m_renderer->getBufferManager();
		GfxApiHandle gfx = m_renderer->getGfxHandle();
		Gfx::setRaytracePipelineState(gfx, exec.cmdBuffer, m_raytracePso);

		DescriptorSetHandle bindings[] = {
			 m_rtCommonResourcesDescSet,
			 texMngr->getTextureArrayDescSet(),
			 buffMngr->getBufferArrayDescSet(),
			 m_rtMiscResourcesDescSet
		};
		Gfx::bindDescriptorSets(gfx, exec.cmdBuffer, BindingPoint::BINDING_POINT_RAYTRACE, m_rtLayout.getPipelineLayoutHandle(), bindings, 0, countOf(bindings), nullptr, 0);

		Gfx::dispatchRays(gfx, exec.cmdBuffer, m_lastUpdateParams.raysPerFrame.x, m_lastUpdateParams.raysPerFrame.y, 1, m_shaderTableHelper.getShaderTable());
	}

}