#include <Renderer/Shared/RenderPipeline/PathTracer/BidirectionalPathIntegratorSubStage.h>
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
#include <Renderer/Shared/LightManager.h>

namespace YAPT
{
	constexpr glm::uvec3 WG_SIZE = glm::uvec3(8, 8, 1);

	BidirectionalPathIntegratorSubStage::BidirectionalPathIntegratorSubStage()
		:m_renderObjectsGPU(RESOURCE_USAGE_COPY_DESTINATION | RESOURCE_USAGE_STORAGE_BUFFER),
		m_constantsGPU(RESOURCE_USAGE_COPY_DESTINATION | RESOURCE_USAGE_UNIFORM_BUFFER)
	{

	}
	BidirectionalPathIntegratorSubStage::~BidirectionalPathIntegratorSubStage()
	{

	}

	void BidirectionalPathIntegratorSubStage::initialize(AccelerationStructureProvider* accStructProvider, CRenderer* rend, RenderGraph* graph, BindlessMaterialManager* matMngr, BindlessMeshManager* meshMngr)
	{
		m_graph = graph;
		m_renderer = rend;
		m_accStructProvider = accStructProvider;
		m_materialMngr = matMngr;
		m_meshMngr = meshMngr;

		RenderGraphNodeSlotDefinition slotdefsCameraRaysNode[] =
		{ 
			{
				ResourceDimension::TEXTURE_2D,
				ResourceFormat::RGBA16_SFLOAT,
				RESOURCE_USAGE_STORAGE_TEXTURE,
				ACCESS_FLAGS_READ_WRITE,
				SHADERSTAGE_RT_RAYGENERATION,
				1,
				1
			},
			{
				RenderGraphBufferSlotDefinition(RESOURCE_USAGE_STORAGE_BUFFER,
				ACCESS_FLAGS_READ,
				SHADERSTAGE_COMPUTE)
			}
		};

		m_cameraPathsNode = m_graph->createComputeNode(2, slotdefsCameraRaysNode, []
		(RenderGraphNode* node, const RenderGraphNodeExecutionContext& execContext, void* usrData)
			{
				static_cast<BidirectionalPathIntegratorSubStage*>(usrData)->executeCameraPathPass(execContext);
			},
			this, "CameraPathsNode");
		

	}
	void BidirectionalPathIntegratorSubStage::shutdown()
	{
		m_raytraceCommon.shutdown();
		m_cameraPathHelperUtility.deinit();
		m_renderObjectsGPU.free();
		m_lightDataGPU.free();


	}
	void BidirectionalPathIntegratorSubStage::onRenderGraphCompiled(const RenderStage::RenderGraphLifetimeData& data)
	{
		ShaderLoader* loader = m_renderer->getShaderLoader();

		BindlessTextureManager* texMngr = m_renderer->getTextureManager();
		BindlessBufferManager* buffMngr = m_renderer->getBufferManager();
		const DescriptorSetLayoutBinding& bindingTex = texMngr->getBindingDefinition();
		const DescriptorSetLayoutBinding& bindingBuff = buffMngr->getBindingDefinition();

		SamplerHandle samplerHandleLinRep = m_renderer->getCoreResources()->getDefaultSampler(DefaultSamplerType::LINEAR_REPEAT);
		SamplerHandle samplerHandleNearestRep = m_renderer->getCoreResources()->getDefaultSampler(DefaultSamplerType::NEAREST_REPEAT);
		SamplerHandle samplerHandleLinClamp = m_renderer->getCoreResources()->getDefaultSampler(DefaultSamplerType::LINEAR_CLAMP);

		ExplicitDescriptorSetDefinition explicitDescSetDefs[] =
		{
			{
				1,
				&bindingTex,
				1,
				texMngr->getLayout()
			},
			{
				2,
				&bindingBuff,
				1,
				buffMngr->getLayout()
			}
		};


		StaticSamplerEntry staticSamplers[] =
		{
			{
				"g_colorSampler",
				samplerHandleLinRep
			},
			{
				"g_pointSampler",
				samplerHandleNearestRep
			},
			{
				"g_lutSampler",
				samplerHandleLinClamp
			},
		};

		
		{
			const ShaderLoader::ShaderPipelineInfo* cameraRays = loader->getShaderPipeline("cameraRaysBDPT");
			m_cameraPathHelperUtility.init(m_renderer, cameraRays, staticSamplers, countOf(staticSamplers), explicitDescSetDefs, countOf(explicitDescSetDefs));
			m_cameraPathHelperUtility.createPipelineState();
		}

		m_renderObjectsGPU.init(m_renderer->getGfxHandle());
		m_lightDataGPU.init(m_renderer->getGfxHandle());
		m_constantsGPU.init(data.renderGraphLifetimeResources);
		m_raytraceCommon.initialize(m_renderer, data.renderGraphLifetimeResources, m_materialMngr, m_meshMngr);

	}
	void BidirectionalPathIntegratorSubStage::onRenderResolutionChanged(const RenderStage::RenderResolutionDependantResourcesData& data, uvec2 newResolution)
	{
		m_renderResolution = newResolution;

		RenderGraphResourceId rtTarget = m_cameraPathsNode->getRenderGraphResourceIdForSlot(0);
		const RenderGraphResourceDescription& desc = m_graph->getRenderGraphResourceDescription(rtTarget);

		TextureDesc textureDesc(desc.resourceDimensions, desc.resourceFormat, desc.resourceUsage, newResolution.x, newResolution.y, 1, 1);
		TextureHandle rtTargetTex = data.resolutionDependantResourcesPool->requestTexture(textureDesc, "Main scene RT Color Target");
		m_graph->setRenderGraphResourceTexture(rtTarget, rtTargetTex);

	}

	void BidirectionalPathIntegratorSubStage::prepare(const RenderStage::PrepareData& params)
	{
		m_accStructProvider->prepareAccelerationStructure();
		setupWorldBoundsJob(params.prepareTasksPool);
	}

	void BidirectionalPathIntegratorSubStage::update(const BidirectionalPathIntegratorSubStage::UpdateParams& params)
	{
		RaytraceCommonResources::UpdateParams p;
		p.rayGenOffsetInTexels = params.rayGenOffsetInTexels;
		p.raysPerFrame = params.raysPerFrame;
		p.renderResolution = m_renderResolution;
		p.sampleOffset = params.sampleOffset;
		m_raytraceCommon.update(p);

		m_lastUpdateParams = params;

		{
			AABB worldBounds = AABB::createEmpty();
			for (size_t i = 0; i < m_combineBoundsJobs.size(); ++i)
			{
				worldBounds.encapsulate(m_combineBoundsJobs[i].combinedBounds);
			}

			BidirectionalPathTraceConstants* constants = m_constantsGPU.getData();
			constants->worldBoundsMax = vec4p(worldBounds.max, 0.f);
			constants->worldBoundsMin = vec4p(worldBounds.min, 0.f);
			m_constantsGPU.flush();
		}
		
		


		setupLightDataJob(params.stageUpdateContext->updateTasksPool);
	}

	void BidirectionalPathIntegratorSubStage::getOutput(RenderGraphNode** node, size_t& slotOut)
	{
		*node = m_cameraPathsNode;
		slotOut = 0;
	}


	void BidirectionalPathIntegratorSubStage::sceneChanged(const RenderObjectId* ids, MaterialPerSubmeshArray* materials, MeshInternal** meshes, size_t* instanceOffsets, size_t objectCount, size_t instancesCount)
	{

		bool emptyScene = instancesCount == 0;
		size_t numberOfEntriesNeeded = max((size_t)1, instancesCount);

		m_renderObjectsGPU.allocate(numberOfEntriesNeeded, "RenderObjectsBuffer");
		char* data = m_renderObjectsGPU.map(0, numberOfEntriesNeeded);

		if (!emptyScene)
		{
			for (size_t i = 0; i < objectCount; ++i)
			{
				size_t entryOffset = instanceOffsets[i];
				for (size_t k = 0; k < meshes[i]->getSubmeshCount(); ++k)
				{
					const MaterialPerSubmeshArray& mat = materials[i];
					const MeshInternal* mesh = meshes[i];

					size_t matId = mat.getMaterialForSubmeshIndex(k)->getID();
					size_t meshId = mesh->getMeshIndex();

					size_t matIndex = m_materialMngr->getEntryIndexForMaterialId(matId);
					size_t meshIndex = m_meshMngr->getEntryIndexForMeshIdAndSubmesh(meshId, k);

					RenderObjectEntry entry;
					entry.materialAndMeshIndices = uvec2p(matIndex, meshIndex);

					memcpy(data + (entryOffset + k) * m_renderObjectsGPU.getAlignedEntrySize(), &entry, sizeof(RenderObjectEntry));

				}

			}
		}
		else
		{
			RenderObjectEntry dummy;
			dummy.materialAndMeshIndices = uvec2p(0, 0);
			memcpy(data, &dummy, sizeof(dummy));
		}

		m_renderObjectsGPU.unmap();

		//update miss
		{
			RCPtr<Texture> skymap = m_renderer->getConcreteRendererConfiguration().getRendererVarValueInternal<Texture*>(RVARNAME_SKYBOX);
			BidirectionalPathTraceConstants* constants = m_constantsGPU.getData();

			bool hasValidEnvtex = false;

			if (skymap != nullptr)
			{
				if (skymap->getDesc().dimension == ResourceDimension::TEXTURE_CUBEMAP)
				{
					hasValidEnvtex = true;
					constants->envType = ENVIRONMENT_TYPE_CUBE;

				}
				else if (skymap->getDesc().dimension == ResourceDimension::TEXTURE_2D)
				{
					hasValidEnvtex = true;
					constants->envType = ENVIRONMENT_TYPE_LONGLAT;
				}
			}

			if (hasValidEnvtex)
			{
				constants->envTextureIndex = static_cast<TextureImpl*>(skymap.get())->getBindlessResourceArrayIndex();
			}
			else
			{
				constants->envType = ENVIRONMENT_TYPE_NONE;
				constants->envTextureIndex = uint32_t(-1);
			}
		}

	}


	void BidirectionalPathIntegratorSubStage::executeCameraPathPass(const RenderGraphNodeExecutionContext& exec)
	{
		{
			m_cameraPathHelperUtility.reserveNewDescriptorSet(0);
			DescriptorSetHandle descSetCommon = m_cameraPathHelperUtility.getDescriptorSet(0);
			m_raytraceCommon.updateCommonResourcesToDescriptorSet(descSetCommon);

			TextureViewHandle rtOutputUav = m_graph->getTextureViewFromNodeSlot(m_cameraPathsNode->getSortedIndex(), 0);
			TopLevelAccelerationStructureHandle accStruct = m_accStructProvider->getAccelerationStructure();
			BufferViewHandle renderObjectsBufferHandle = m_renderObjectsGPU.getBufferViewHandle();
			BufferViewHandle lightsBufferHandle = m_lightDataGPU.getBufferViewHandle();

			DescriptorSetUpdate updates[] = {
				{0, 0, 1, DescriptorPtr(m_constantsGPU.getViewPtr())},
				{1, 0, 1, DescriptorPtr(&renderObjectsBufferHandle)},
				{2, 0, 1, DescriptorPtr(&lightsBufferHandle)},
				{3, 0, 1, DescriptorPtr(&accStruct)},
				{4, 0, 1, DescriptorPtr(&rtOutputUav)},
			};

			m_cameraPathHelperUtility.reserveAndUpdateDescriptorSet(3, updates, countOf(updates));

			BindlessTextureManager* texMngr = m_renderer->getTextureManager();
			BindlessBufferManager* buffMngr = m_renderer->getBufferManager();

			m_cameraPathHelperUtility.setExternallyOwnedDescriptorSet(1, texMngr->getTextureArrayDescSet());
			m_cameraPathHelperUtility.setExternallyOwnedDescriptorSet(2, buffMngr->getBufferArrayDescSet());

		}

		glm::uvec3 dispatchArgs = DivRoundUp(glm::uvec3(m_lastUpdateParams.raysPerFrame.x, m_lastUpdateParams.raysPerFrame.y, 1), WG_SIZE);

		m_cameraPathHelperUtility.dispatch(exec.cmdBuffer, dispatchArgs.x, dispatchArgs.y, dispatchArgs.z);;
		

	}

	void BidirectionalPathIntegratorSubStage::setupWorldBoundsJob(ThreadPool* threadPool)
	{
		constexpr size_t MIN_ITEMS_PER_JOB = 100;
		const AABB* objectBounds = m_renderer->getRenderObjectManager().getAllBounds();
		size_t count = m_renderer->getRenderObjectManager().getNumberOfObjects();
		size_t numberOfJobs = max(size_t(1), min(size_t(m_combineBoundsJobs.size()), count / MIN_ITEMS_PER_JOB));
		size_t operationsPerJob = (count + numberOfJobs - 1) / numberOfJobs;

		if (count == 0)
		{
			return;
		}

		auto combineBoundsJob = [](void* usrData)
		{
			CombineBoundsJobItem* item = static_cast<CombineBoundsJobItem*>(usrData);
			AABB combinedBounds = AABB::createEmpty();
			for (size_t i = 0; i < item->boundsCount; ++i)
			{
				size_t index = item->boundsOffset + i;
				const AABB& b = item->objectBounds[index];

				combinedBounds.encapsulate(b);
			}

			item->combinedBounds = combinedBounds;

		};

		size_t offset = 0;
		for (size_t i = 0; i < m_combineBoundsJobs.size(); ++i)
		{
			CombineBoundsJobItem& item = m_combineBoundsJobs[i];
			item.objectBounds = objectBounds;
			item.boundsOffset = offset;
			item.boundsCount = min(operationsPerJob, count - offset);
			item.combinedBounds = AABB::createEmpty();
			offset += item.boundsCount;

		}


		for (size_t i = 0; i < numberOfJobs; ++i)
		{
			threadPool->addTask(combineBoundsJob, &m_combineBoundsJobs[i]);
		}
	}
	void BidirectionalPathIntegratorSubStage::setupLightDataJob(ThreadPool* threadPool)
	{
		
		auto uploadLightDataJob = [](void* usrData)
		{
			BidirectionalPathIntegratorSubStage* subStage = static_cast<BidirectionalPathIntegratorSubStage*>(usrData);
			LightManager* lightManager = subStage->m_renderer->getLightManager();
			DynamicSizeGpuBufferHelper<LightEntryGPU> lightDataGPU = subStage->m_lightDataGPU;
			RenderObjectManager& roMngr = subStage->m_renderer->getRenderObjectManager();
			BindlessMeshManager* meshMngr = subStage->m_meshMngr;
			BindlessMaterialManager* matMngr = subStage->m_materialMngr;

			size_t lightCount = lightManager->getLightReferenceCount();
			if (lightCount == 0)
			{
				if (lightDataGPU.getAllocatedEntryCount() == 0)
				{
					lightDataGPU.allocate(1, "EmissiveObjectsReferences");
				}
				return;
			}

			if (lightCount > lightDataGPU.getAllocatedEntryCount())
			{
				lightDataGPU.allocate(lightCount, "EmissiveObjectsReferences");
			}

			char* dstPtr = lightDataGPU.map(0, lightCount);
			const LightManager::LightReference* refs = lightManager->getAllLightReferences();
			for (size_t i = 0; i < lightCount; ++i)
			{
				const LightManager::LightReference& ref = refs[i];
				const mat4& t = roMngr.getMatrixForId(ref.objectId);
				size_t meshIndex = meshMngr->getEntryIndexForMeshIdAndSubmesh(ref.objectId, ref.submeshIndex);
				size_t matIndex = matMngr->getEntryIndexForMaterialId(roMngr.getMaterialForId(ref.objectId).getMaterialForSubmeshIndex(ref.submeshIndex)->getID());

				LightEntryGPU gpuEntry;
				gpuEntry.transform = t;
				gpuEntry.meshIndex = (uint32_t)meshIndex;
				gpuEntry.matIndex = (uint32_t)matIndex;
				

				memcpy(dstPtr + i * lightDataGPU.getAlignedEntrySize(), &gpuEntry, sizeof(LightEntryGPU));
			}


			lightDataGPU.unmap();
		};

		
		threadPool->addTask(uploadLightDataJob, this);
		
	}

}