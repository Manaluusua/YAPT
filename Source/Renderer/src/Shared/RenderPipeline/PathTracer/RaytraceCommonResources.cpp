#include <Renderer/Shared/RenderPipeline/PathTracer/RaytraceCommonResources.h>
#include <Renderer/Shared/CRenderer.h>
#include <Renderer/Shared/Utility/CoreRenderResourcesUtility.h>
#include <Renderer/Shared/Utility/RenderAPIAbstractionUtility.h>
#include <Renderer/Shared/BindlessMaterialManager.h>
#include <Renderer/Shared/BindlessMeshManager.h>
#include <Renderer/Shared/LightManager.h>
#include <Math/MathUtility.h>
#include <Math/RandUtility.h>
#include <Renderer/Shared/Utility/SpectralUtility.h>
#include <Renderer/Shared/TextureImpl.h>
#include <array>

namespace YAPT
{
	

	RaytraceCommonResources::RaytraceCommonResources()
		:m_renderObjectMaterialAndMeshIndices(RESOURCE_USAGE_COPY_DESTINATION | RESOURCE_USAGE_STORAGE_BUFFER, true),
		m_renderObjectTransformData(RESOURCE_USAGE_COPY_DESTINATION | RESOURCE_USAGE_STORAGE_BUFFER),
		m_lightDataGPU(RESOURCE_USAGE_COPY_DESTINATION | RESOURCE_USAGE_STORAGE_BUFFER),
		m_randomSamples(RESOURCE_USAGE_COPY_DESTINATION | RESOURCE_USAGE_STORAGE_BUFFER, true)
	{

	}

	RaytraceCommonResources::~RaytraceCommonResources()
	{

	}

	void RaytraceCommonResources::initialize( CRenderer* rend, RenderResourcesPool* resourcesPool, BindlessMaterialManager* matMngr, BindlessMeshManager* meshMngr)
	{
		m_renderer = rend;
		m_materialMngr = matMngr;
		m_meshMngr = meshMngr;

		m_rayTraceConstants.init(resourcesPool);
		m_randomSamples.init(resourcesPool);

		m_renderObjectMaterialAndMeshIndices.init(m_renderer->getGfxHandle());
		m_renderObjectTransformData.init(m_renderer->getGfxHandle());
		m_lightDataGPU.init(m_renderer->getGfxHandle());

	}
	void RaytraceCommonResources::shutdown()
	{
		m_renderObjectMaterialAndMeshIndices.free();
		m_renderObjectTransformData.free();
		m_lightDataGPU.free();
	}

	JobHandle RaytraceCommonResources::prepare(const RaytraceCommonResources::PrepareParams& params)
	{
		JobHandle jobsToWait[1];
		jobsToWait[0] = setupWorldBoundsJob(params.prepareTasksPool);
		return params.prepareTasksPool->combineDependencies(jobsToWait, countOf(jobsToWait));
	}

	JobHandle RaytraceCommonResources::update(const RaytraceCommonResources::UpdateParams& params)
	{
		RaytraceConstantData* rtConstants = m_rayTraceConstants.getData();
		float targetPixelWidth = 1.f / params.renderResolution.x;
		float targetPixelHeight = 1.f / params.renderResolution.y;

		//ray offset
		vec2p rayUVOffset(0.5 * targetPixelWidth, 0.5 * targetPixelHeight); //move to pixel center

		vec4 camPos(0.f, 0.f, 0.f, 1.f);
		mat4 worldToView = m_renderer->getCurrentRenderView().getView();
		mat4 viewToWorld = glm::inverse(worldToView);
		camPos = viewToWorld * camPos;

		uint32_t flags = 0;

		//first frame writes out material outputs for denoising (and disables subpixel jitter to get values from the center of the texel)
		if (params.sampleOffset == 0)
		{
			flags |= CONSTANTS_FLAG_WRITE_FIRST_BOUNCE_MATERIAL_PARAMS;
			flags |= CONSTANTS_FLAG_DISABLE_TEXEL_JITTER;
		}

		rtConstants->maxRayDepth = 16;
		rtConstants->rayUVOffset = rayUVOffset;
		rtConstants->cameraPosition = camPos;
		rtConstants->targetTexDimensions = vec4p(params.renderResolution.x, params.renderResolution.y, targetPixelWidth, targetPixelHeight);
		rtConstants->targetOffsetScaleBias = params.targetOffsetScaleBias;
		rtConstants->flags = flags;

		mat4 viewToUVTransform = fromPlatformNDCToTextureSpace() * m_renderer->getCurrentRenderView().getProjectionPlatform();
		mat4 uvToViewTransform = glm::inverse(viewToUVTransform);
		

		rtConstants->uvToView = uvToViewTransform;
		rtConstants->viewToWorld = viewToWorld;
		rtConstants->viewToUV = viewToUVTransform;
		rtConstants->worldToView = worldToView;
		rtConstants->lightCount = (uint32_t)m_renderer->getLightManager()->getLightReferenceCount();

		rtConstants->envIntensityScale = 1;

		//update samples
		if ((params.sampleOffset % NUMBER_OF_RANDOM_SAMPLES) == 0)
		{
			updateSamples(params.sampleOffset);
		}


		{
			
			rtConstants->worldBoundsMax = vec4p(m_worldBounds.max, 0.f);
			rtConstants->worldBoundsMin = vec4p(m_worldBounds.min, 0.f);
		}

		m_rayTraceConstants.flush();

		JobHandle jobsToWait[1];
		jobsToWait[0] = setupLightDataJob(params.updateTasksPool); //need to schedule this in update for now since bindlessmaterialmanager gets updated in update: once its jobified can move this earlier
		return params.updateTasksPool->combineDependencies(jobsToWait, countOf(jobsToWait));
	}

	void RaytraceCommonResources::setupCommonSamplers(CRenderer* rend, PipelineLayoutHelper& helper, const ShaderPipelineReflection& refl, ShaderModuleType mod)
	{
		{
			SamplerHandle samplerHandleLinRep = rend->getCoreResources()->getDefaultSampler(DefaultSamplerType::LINEAR_REPEAT);
			SamplerHandle samplerHandleNearestRep = rend->getCoreResources()->getDefaultSampler(DefaultSamplerType::NEAREST_REPEAT);
			SamplerHandle samplerHandleLinClamp = rend->getCoreResources()->getDefaultSampler(DefaultSamplerType::LINEAR_CLAMP);

			helper.setStaticSamplers("g_colorSampler", refl, mod, &samplerHandleLinRep);
			helper.setStaticSamplers("g_pointSampler", refl, mod, &samplerHandleNearestRep);
			helper.setStaticSamplers("g_lutSampler", refl, mod, &samplerHandleLinClamp);
		}
	}

	void RaytraceCommonResources::updateCommonResourcesToDescriptorSet(DescriptorSetHandle handle)
	{
		BufferViewHandle meshEntriesBuffer = m_meshMngr->getBufferViewHandle();
		BufferViewHandle materialEntriesBuffer = m_materialMngr->getBufferViewHandle();
		BufferViewHandle renderObjectsMatMeshBufferHandle = m_renderObjectMaterialAndMeshIndices.getBufferViewHandle();
		BufferViewHandle renderObjectsTransformBufferHandle = m_renderObjectTransformData.getBufferViewHandle();
		BufferViewHandle lightsBufferHandle = m_lightDataGPU.getBufferViewHandle();

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
			{3, 0, 1, DescriptorPtr(&renderObjectsTransformBufferHandle)},
			{4, 0, 1, DescriptorPtr(&renderObjectsMatMeshBufferHandle)},
			{5, 0, 1, DescriptorPtr(&lightsBufferHandle)},
			{6, 0, 1, DescriptorPtr(&materialEntriesBuffer)},
			{7, 0, 1, DescriptorPtr(&meshEntriesBuffer)},
			

			{12, 0, 1, DescriptorPtr(&noiseTex)},

			{13, 0, 1, DescriptorPtr(&singleScatterAlbedoNoFresnel)},
			{14, 0, 1, DescriptorPtr(&singleScatterAverageAlbedoNoFresnel)},
			{15, 0, 1, DescriptorPtr(&singleAndMultiScatterAlbedo)},
			{16, 0, 1, DescriptorPtr(&singleAndMultiScatterAverageAlbedo)},
			{17, 0, 1, DescriptorPtr(&singleScatterAlbedoTranslucentDenser)},
			{18, 0, 1, DescriptorPtr(&singleScatterAverageAlbedoTranslucentLighter)},
			{19, 0, 1, DescriptorPtr(&singleScatterAvgAlbedoTranslucentDenser)},
			{20, 0, 1, DescriptorPtr(&singleScatterAvgAverageAlbedoTranslucentLighter)},
			{21, 0, 1, DescriptorPtr(&sheenDirectionalAlbedo)},

			{22, 0, 1, DescriptorPtr(&cieLUT)},
			{23, 0, 1, DescriptorPtr(&d65LUT)},
			{24, 0, 1, DescriptorPtr(&toREC2020LUT)},
			{25, 0, 1, DescriptorPtr(&toSRGBLUT)},
		};

		Gfx::updateDescriptorSet(m_renderer->getGfxHandle(), handle, updates, countOf(updates));
	}

	void RaytraceCommonResources::sceneChanged(const PathIntegratorSubStage::SceneData& sceneData)
	{
		bool emptyScene = sceneData.instancesCount == 0;
		size_t numberOfEntriesNeeded = max((size_t)1, sceneData.instancesCount);
		MeshManager& meshMngr = m_renderer->getMeshManager();

		m_renderObjectMaterialAndMeshIndices.allocate(numberOfEntriesNeeded, "RenderObjectsMaterialAndMeshIndicesBuffer");
		m_renderObjectTransformData.allocate(numberOfEntriesNeeded, "RenderObjectsTransformsBuffer");
		char* matAndMeshData = m_renderObjectMaterialAndMeshIndices.map(0, numberOfEntriesNeeded);
		char* transformsData = m_renderObjectTransformData.map(0, numberOfEntriesNeeded);
		m_instanceOffsetPerRenderObject.resize(sceneData.objectCount); //copy offsets

		if (!emptyScene)
		{
			for (size_t i = 0; i < sceneData.objectCount; ++i)
			{
				MeshInternal* mesh = meshMngr.getMeshInternal(sceneData.meshes[i]);
				size_t entryOffset = sceneData.instanceOffsetPerRenderObject[i];
				m_instanceOffsetPerRenderObject[i] = entryOffset;

				//glm keeps things in column major order, tranpose to get rows out easily
				mat4 wMatT = glm::transpose(sceneData.objToWorldMatrices[i]);
				mat4 wMatInvT = glm::transpose(sceneData.worldToObjMatrices[i]);

				RenderObjectTransformDataGPU transformDataGPU;
				transformDataGPU.objToWorldR0 = wMatT[0];
				transformDataGPU.objToWorldR1 = wMatT[1];
				transformDataGPU.objToWorldR2 = wMatT[2];
				transformDataGPU.worldToObjectR0 = wMatInvT[0];
				transformDataGPU.worldToObjectR1 = wMatInvT[1];
				transformDataGPU.worldToObjectR2 = wMatInvT[2];

				for (size_t k = 0; k < mesh->getSubmeshCount(); ++k)
				{
					size_t instanceIndex = entryOffset + k;
					const MaterialPerSubmeshArray& mat = sceneData.materials[i];


					size_t matId = mat.getMaterialIDForSubmeshIndex(k);
					size_t meshId = mesh->getID();

					size_t matIndex = m_materialMngr->getEntryIndexForMaterialId(matId);
					size_t meshIndex = m_meshMngr->getEntryIndexForMeshIdAndSubmesh(meshId, k);

					
					uvec2p materialAndMeshIndices = uvec2p(matIndex, meshIndex);

					

					memcpy(matAndMeshData + (instanceIndex) * sizeof(uvec2p), &materialAndMeshIndices, sizeof(uvec2p));
					memcpy(transformsData + (instanceIndex) * sizeof(RenderObjectTransformDataGPU), &transformDataGPU, sizeof(RenderObjectTransformDataGPU));

				}

			}
		}
		else
		{
			memset(matAndMeshData, 0, sizeof(uvec2p));
			memset(transformsData, 0, sizeof(RenderObjectTransformDataGPU));
		}

		m_renderObjectMaterialAndMeshIndices.unmap();
		m_renderObjectTransformData.unmap();


		//update miss
		{
			RCPtr<Texture> skymap = m_renderer->getConcreteRendererConfiguration().getRendererVarValueInternal<Texture*>(RVARNAME_SKYBOX);
			RaytraceConstantData* rtConstants = m_rayTraceConstants.getData();

			bool hasValidEnvtex = false;

			if (skymap != nullptr)
			{
				if (skymap->getDesc().dimension == ResourceDimension::TEXTURE_CUBEMAP)
				{
					hasValidEnvtex = true;
					rtConstants->envType = ENVIRONMENT_TYPE_CUBE;

				}
				else if (skymap->getDesc().dimension == ResourceDimension::TEXTURE_2D)
				{
					hasValidEnvtex = true;
					rtConstants->envType = ENVIRONMENT_TYPE_LONGLAT;
				}
			}

			if (hasValidEnvtex)
			{
				rtConstants->envTextureIndex = static_cast<TextureImpl*>(skymap.get())->getBindlessResourceArrayIndex();
			}
			else
			{
				rtConstants->envType = ENVIRONMENT_TYPE_NONE;
				rtConstants->envTextureIndex = uint32_t(-1);
			}
		}

	}

	JobHandle RaytraceCommonResources::setupLightDataJob(JobSystem* threadPool)
	{

		auto uploadLightDataJob = [](void* usrData)
		{
			RaytraceCommonResources* subStage = static_cast<RaytraceCommonResources*>(usrData);
			LightManager* lightManager = subStage->m_renderer->getLightManager();
			DynamicSizeGpuBufferHelper<LightEntryGPU>& lightDataGPU = subStage->m_lightDataGPU;
			RenderObjectManager& roMngr = subStage->m_renderer->getRenderObjectManager();
			BindlessMeshManager* bindlessMeshMngr = subStage->m_meshMngr;
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
				MeshIndex meshId = roMngr.getMeshForId(ref.objectId);
				const mat4& t = roMngr.getWorldMatrixForId(ref.objectId);
				mat4p invTransp = glm::transpose(glm::inverse(t));
				size_t meshIndex = bindlessMeshMngr->getEntryIndexForMeshIdAndSubmesh(meshId, ref.submeshIndex);
				size_t matIndex = matMngr->getEntryIndexForMaterialId(roMngr.getMaterialForId(ref.objectId).getMaterialIDForSubmeshIndex(ref.submeshIndex));
				AABB bounds = roMngr.getBoundsForId(ref.objectId);
				vec3p c = bounds.center();
				float radius = bounds.radius();
				size_t renderObjectDataIndex = roMngr.getDataIndexForRenderObjectId(ref.objectId);

				LightEntryGPU gpuEntry;
				gpuEntry.transform = t;
				gpuEntry.transformInvTransp = invTransp;
				gpuEntry.meshIndex = (uint32_t)meshIndex;
				gpuEntry.matIndex = (uint32_t)matIndex;
				gpuEntry.centerRadius = vec4p(c.x, c.y, c.z, radius);
				gpuEntry.instanceIndex = uint32_t(subStage->m_instanceOffsetPerRenderObject[renderObjectDataIndex] + ref.submeshIndex); 

				memcpy(dstPtr + i * lightDataGPU.getAlignedEntrySize(), &gpuEntry, sizeof(LightEntryGPU));
			}


			lightDataGPU.unmap();
		};


		return threadPool->submit(uploadLightDataJob, this);
	}


	JobHandle RaytraceCommonResources::setupWorldBoundsJob(JobSystem* threadPool)
	{
		constexpr size_t MIN_ITEMS_PER_JOB = 100;
		const AABB* objectBounds = m_renderer->getRenderObjectManager().getAllBounds();
		size_t count = m_renderer->getRenderObjectManager().getNumberOfObjects();
		size_t numberOfJobs = max(size_t(1), min(size_t(m_combineBoundsJobs.size()), count / MIN_ITEMS_PER_JOB));
		size_t operationsPerJob = (count + numberOfJobs - 1) / numberOfJobs;

		std::array<JobHandle, COMBINE_BOUNDS_MAX_JOBS> deps;

		if (count == 0)
		{
			return {};
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

		auto combineBoundsFinalJob = [](void* usrData)
		{
			RaytraceCommonResources* res = static_cast<RaytraceCommonResources*>(usrData);
			AABB worldBounds = AABB::createEmpty();
			for (size_t i = 0; i < res->m_combineBoundsJobs.size(); ++i)
			{
				worldBounds.encapsulate(res->m_combineBoundsJobs[i].combinedBounds);
			}
			res->m_worldBounds = worldBounds;
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
			deps[i] = threadPool->submit(combineBoundsJob, &m_combineBoundsJobs[i]);
		}

		return threadPool->submit(combineBoundsFinalJob, this, deps.data(), numberOfJobs);
	}

	void RaytraceCommonResources::updateSamples(size_t sampleOffset)
	{
		MathUtils::generateSobolSequence(NUMBER_OF_RANDOM_SAMPLES, NUMBER_OF_RANDOM_SAMPLE_DIMENSIONS,  m_randomSamples.getData()->samples, (uint32_t)sampleOffset);
		m_randomSamples.flush();
	}
}