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
	constexpr glm::uvec3 WG_SIZE_CAM_STAGE = glm::uvec3(8, 8, 1);
	constexpr glm::uvec3 WG_SIZE_LIGHT_STAGE = glm::uvec3(64, 1, 1);
	constexpr uint32_t MAX_VERTICES_PER_LIGHT_PATH = 8;
	constexpr float ACTUAL_ALLOCATED_VERTICES_PER_PATH_RATIO = 0.8f;
	constexpr glm::uvec2 TEXELS_PER_LIGHTPATH = glm::uvec2(8, 8);

	BidirectionalPathIntegratorSubStage::BidirectionalPathIntegratorSubStage()
		:m_constantsGPU(RESOURCE_USAGE_COPY_DESTINATION | RESOURCE_USAGE_UNIFORM_BUFFER),
		m_lightPathHeadersGPU(RESOURCE_USAGE_STORAGE_BUFFER),
		m_lightPathsGPU(RESOURCE_USAGE_STORAGE_BUFFER),
		m_countersGPU(RESOURCE_USAGE_STORAGE_BUFFER | RESOURCE_USAGE_COPY_DESTINATION),
		m_maxVerticesPerLightPath(MAX_VERTICES_PER_LIGHT_PATH),
		m_pixelsPerLightPath(TEXELS_PER_LIGHTPATH)

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

		RenderGraphNodeSlotDefinition slotdefsLightPaths[] =
		{
			{
				RenderGraphBufferSlotDefinition(RESOURCE_USAGE_STORAGE_BUFFER,
				ACCESS_FLAGS_WRITE,
				SHADERSTAGE_COMPUTE)
			},
			{
				RenderGraphBufferSlotDefinition(RESOURCE_USAGE_STORAGE_BUFFER,
				ACCESS_FLAGS_WRITE,
				SHADERSTAGE_COMPUTE)
			},
			{
				RenderGraphBufferSlotDefinition(RESOURCE_USAGE_STORAGE_BUFFER,
				ACCESS_FLAGS_READ_WRITE,
				SHADERSTAGE_COMPUTE)
			},
		};

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
			},
			{
				RenderGraphBufferSlotDefinition(RESOURCE_USAGE_STORAGE_BUFFER,
				ACCESS_FLAGS_READ,
				SHADERSTAGE_COMPUTE)
			},
			{
				RenderGraphBufferSlotDefinition(RESOURCE_USAGE_STORAGE_BUFFER,
				ACCESS_FLAGS_READ,
				SHADERSTAGE_COMPUTE)
			}
		};

		m_lightPathsNode = m_graph->createComputeNode(3, slotdefsLightPaths, []
		(RenderGraphNode* node, const RenderGraphNodeExecutionContext& execContext, void* usrData)
			{
				static_cast<BidirectionalPathIntegratorSubStage*>(usrData)->executeLightPathPass(execContext);
			},
			this, "LightPathsNode");

		m_cameraPathsNode = m_graph->createComputeNode(4, slotdefsCameraRaysNode, []
		(RenderGraphNode* node, const RenderGraphNodeExecutionContext& execContext, void* usrData)
			{
				static_cast<BidirectionalPathIntegratorSubStage*>(usrData)->executeCameraPathPass(execContext);
			},
			this, "CameraPathsNode");
		
		m_graph->createEdge(m_lightPathsNode, 0, m_cameraPathsNode, 1);
		m_graph->createEdge(m_lightPathsNode, 1, m_cameraPathsNode, 2);
		m_graph->createEdge(m_lightPathsNode, 2, m_cameraPathsNode, 3);
	}
	void BidirectionalPathIntegratorSubStage::shutdown()
	{
		m_raytraceCommon.shutdown();
		m_cameraPathHelperUtility.deinit();
		m_lightPathHelperUtility.deinit();
		m_lightPathHeadersGPU.free();
		m_lightPathsGPU.free();

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
			const ShaderLoader::ShaderPipelineInfo* lightRays = loader->getShaderPipeline("lightRaysBDPT");
			//m_lightPathHelperUtility.init(m_renderer, lightRays,staticSamplers, countOf(staticSamplers), explicitDescSetDefs, countOf(explicitDescSetDefs));
			//m_lightPathHelperUtility.createPipelineState();
		}

		{
			const ShaderLoader::ShaderPipelineInfo* cameraRays = loader->getShaderPipeline("cameraRaysBDPT");
			m_cameraPathHelperUtility.init(m_renderer, cameraRays, staticSamplers, countOf(staticSamplers), explicitDescSetDefs, countOf(explicitDescSetDefs));
			m_cameraPathHelperUtility.createPipelineState();
		}

		m_constantsGPU.init(data.renderGraphLifetimeResources);

		m_lightPathHeadersGPU.init(m_renderer->getGfxHandle());
		m_lightPathsGPU.init(m_renderer->getGfxHandle());
		m_countersGPU.init(data.renderGraphLifetimeResources);

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

		//dummy for now
		{
			RenderGraphResourceId lightPathHeadersId = m_lightPathsNode->getRenderGraphResourceIdForSlot(0);
			RenderGraphResourceId lightPathsNodesId = m_lightPathsNode->getRenderGraphResourceIdForSlot(1);
			RenderGraphResourceId countersBufferId = m_lightPathsNode->getRenderGraphResourceIdForSlot(2);

			uvec2 lightPathCountPerDim = (m_renderResolution + m_pixelsPerLightPath - uvec2(1, 1)) / m_pixelsPerLightPath;
			uint32_t allocatedVertices = lightPathCountPerDim.x * lightPathCountPerDim.y * (uint32_t)glm::round(m_maxVerticesPerLightPath * ACTUAL_ALLOCATED_VERTICES_PER_PATH_RATIO);

			m_constantsGPU.getData()->maxVerticesPerLightPath = m_maxVerticesPerLightPath;
			m_constantsGPU.getData()->lightPathsPerDim = lightPathCountPerDim;
			m_constantsGPU.getData()->maxAllocatedVertices = allocatedVertices;

			m_lightPathHeadersGPU.allocate(lightPathCountPerDim.x * lightPathCountPerDim.y, "lightPathHeaders");
			m_lightPathsGPU.allocate(allocatedVertices, "lightPathNodes");

			m_graph->setRenderGraphResourceBuffer(lightPathHeadersId, m_lightPathHeadersGPU.getBufferHandle());
			m_graph->setRenderGraphResourceBuffer(lightPathsNodesId, m_lightPathsGPU.getBufferHandle());
			m_graph->setRenderGraphResourceBuffer(countersBufferId, m_countersGPU.getBufferHandle());
		}

	}

	void BidirectionalPathIntegratorSubStage::prepare(const RenderStage::PrepareData& params)
	{
		m_accStructProvider->prepareAccelerationStructure();
		setupWorldBoundsJob(params.prepareTasksPool);

		RaytraceCommonResources::PrepareParams prepareParams;
		prepareParams.prepareTasksPool = params.prepareTasksPool;
		m_raytraceCommon.prepare(prepareParams);
	}

	void BidirectionalPathIntegratorSubStage::update(const BidirectionalPathIntegratorSubStage::UpdateParams& params)
	{
		RaytraceCommonResources::UpdateParams p;
		p.rayGenOffsetInTexels = params.rayGenOffsetInTexels;
		p.raysPerFrame = params.raysPerFrame;
		p.renderResolution = m_renderResolution;
		p.sampleOffset = params.sampleOffset;
		p.spectralSampleOffset = params.sampleOffset; //TODO: should make sure that light paths and camera paths share the same wavelength sampleset and maybe try reuse some (?)
		p.updateTasksPool = params.stageUpdateContext->updateTasksPool;
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
		
		m_countersGPU.getData()[0] = uvec2(0, 0);
		m_countersGPU.flush();


		
	}

	void BidirectionalPathIntegratorSubStage::getOutput(RenderGraphNode** node, size_t& slotOut)
	{
		*node = m_cameraPathsNode;
		slotOut = 0;
	}


	void BidirectionalPathIntegratorSubStage::sceneChanged(const RenderObjectId* ids, MaterialPerSubmeshArray* materials, MeshIndex* meshes, size_t* instanceOffsets, size_t objectCount, size_t instancesCount)
	{

		m_raytraceCommon.sceneChanged(ids, materials, meshes, instanceOffsets, objectCount, instancesCount);

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
	


	void BidirectionalPathIntegratorSubStage::executeLightPathPass(const RenderGraphNodeExecutionContext& exec)
	{

	}

	void BidirectionalPathIntegratorSubStage::executeCameraPathPass(const RenderGraphNodeExecutionContext& exec)
	{
		{
			m_cameraPathHelperUtility.reserveNewDescriptorSet(0);
			DescriptorSetHandle descSetCommon = m_cameraPathHelperUtility.getDescriptorSet(0);
			m_raytraceCommon.updateCommonResourcesToDescriptorSet(descSetCommon);

			TextureViewHandle rtOutputUav = m_graph->getTextureViewFromNodeSlot(m_cameraPathsNode->getSortedIndex(), 0);
			TopLevelAccelerationStructureHandle accStruct = m_accStructProvider->getAccelerationStructure();
			

			BufferViewHandle lightPathHeaders = m_lightPathHeadersGPU.getBufferViewHandle();
			BufferViewHandle lightPathVertices = m_lightPathsGPU.getBufferViewHandle();
			BufferViewHandle counters = m_countersGPU.getView();

			DescriptorSetUpdate updates[] = {
				{0, 0, 1, DescriptorPtr(m_constantsGPU.getViewPtr())},
				{1, 0, 1, DescriptorPtr(&accStruct)},
				{2, 0, 1, DescriptorPtr(&lightPathHeaders)},
				{3, 0, 1, DescriptorPtr(&lightPathVertices)},
				{4, 0, 1, DescriptorPtr(&counters)},
				{5, 0, 1, DescriptorPtr(&rtOutputUav)},
			};

			m_cameraPathHelperUtility.reserveAndUpdateDescriptorSet(3, updates, countOf(updates));

			BindlessTextureManager* texMngr = m_renderer->getTextureManager();
			BindlessBufferManager* buffMngr = m_renderer->getBufferManager();

			m_cameraPathHelperUtility.setExternallyOwnedDescriptorSet(1, texMngr->getTextureArrayDescSet());
			m_cameraPathHelperUtility.setExternallyOwnedDescriptorSet(2, buffMngr->getBufferArrayDescSet());

		}

		glm::uvec3 dispatchArgs = DivRoundUp(glm::uvec3(m_lastUpdateParams.raysPerFrame.x, m_lastUpdateParams.raysPerFrame.y, 1), WG_SIZE_CAM_STAGE);

		m_cameraPathHelperUtility.dispatch(exec.cmdBuffer, dispatchArgs.x, dispatchArgs.y, dispatchArgs.z);;


	}


}