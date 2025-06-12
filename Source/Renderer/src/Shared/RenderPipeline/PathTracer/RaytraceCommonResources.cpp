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
#include <array>

namespace YAPT
{
	

	RaytraceCommonResources::RaytraceCommonResources()
		:m_renderObjectsGPU(RESOURCE_USAGE_COPY_DESTINATION | RESOURCE_USAGE_STORAGE_BUFFER),
		m_lightDataGPU(RESOURCE_USAGE_COPY_DESTINATION | RESOURCE_USAGE_STORAGE_BUFFER)
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

		initSubpixelJitterSamples();
		m_rayTraceConstants.init(resourcesPool);
		m_randomSamples.init(resourcesPool);
		m_spectralDataConstants.init(resourcesPool);


		m_renderObjectsGPU.init(m_renderer->getGfxHandle());
		m_lightDataGPU.init(m_renderer->getGfxHandle());

	}
	void RaytraceCommonResources::shutdown()
	{
		m_renderObjectsGPU.free();
		m_lightDataGPU.free();
	}

	void RaytraceCommonResources::prepare(const RaytraceCommonResources::PrepareParams& params)
	{

	}

	void RaytraceCommonResources::update(const RaytraceCommonResources::UpdateParams& params)
	{
		RaytraceConstantData* rtConstants = m_rayTraceConstants.getData();
		float targetPixelWidth = 1.f / params.renderResolution.x;
		float targetPixelHeight = 1.f / params.renderResolution.y;

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
		rtConstants->targetTexDimensions = vec4p(params.renderResolution.x, params.renderResolution.y, targetPixelWidth, targetPixelHeight);

		mat4 uvToViewTransform = glm::inverse(fromPlatformNDCToTextureSpace() * m_renderer->getCurrentRenderView().getProjectionPlatform());

		rtConstants->uvToView = uvToViewTransform;
		rtConstants->viewToWorld = viewToWorld;
		rtConstants->lightCount = (uint32_t)m_renderer->getLightManager()->getLightReferenceCount();

		//update samples
		updateSampledWavelengths(params.spectralSampleOffset);
		if ((params.sampleOffset % NUMBER_OF_RANDOM_SAMPLES) == 0)
		{
			updateSamples(params.sampleOffset);
		}

		setupLightDataJob(params.updateTasksPool);

		m_rayTraceConstants.flush();

		
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
		BufferViewHandle renderObjectsBufferHandle = m_renderObjectsGPU.getBufferViewHandle();
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
			{3, 0, 1, DescriptorPtr(m_spectralDataConstants.getViewPtr())},
			{4, 0, 1, DescriptorPtr(&materialEntriesBuffer)},
			{5, 0, 1, DescriptorPtr(&meshEntriesBuffer)},
			{6, 0, 1, DescriptorPtr(&renderObjectsBufferHandle)},
			{7, 0, 1, DescriptorPtr(&lightsBufferHandle)},

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

		Gfx::updateDescriptorSet(m_renderer->getGfxHandle(), handle, updates, countOf(updates));
	}

	void RaytraceCommonResources::sceneChanged(const RenderObjectId* ids, MaterialPerSubmeshArray* materials, MeshInternal** meshes, size_t* instanceOffsets, size_t objectCount, size_t instancesCount)
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

	}

	void RaytraceCommonResources::setupLightDataJob(ThreadPool* threadPool)
	{

		auto uploadLightDataJob = [](void* usrData)
		{
			RaytraceCommonResources* subStage = static_cast<RaytraceCommonResources*>(usrData);
			LightManager* lightManager = subStage->m_renderer->getLightManager();
			DynamicSizeGpuBufferHelper<LightEntryGPU>& lightDataGPU = subStage->m_lightDataGPU;
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
				mat4p invTransp = glm::transpose(glm::inverse(t));
				size_t meshIndex = meshMngr->getEntryIndexForMeshIdAndSubmesh(ref.objectId, ref.submeshIndex);
				size_t matIndex = matMngr->getEntryIndexForMaterialId(roMngr.getMaterialForId(ref.objectId).getMaterialForSubmeshIndex(ref.submeshIndex)->getID());

				LightEntryGPU gpuEntry;
				gpuEntry.transform = t;
				gpuEntry.transformInvTransp = invTransp;
				gpuEntry.meshIndex = (uint32_t)meshIndex;
				gpuEntry.matIndex = (uint32_t)matIndex;


				memcpy(dstPtr + i * lightDataGPU.getAlignedEntrySize(), &gpuEntry, sizeof(LightEntryGPU));
			}


			lightDataGPU.unmap();
		};


		threadPool->addTask(uploadLightDataJob, this);

	}

	void RaytraceCommonResources::updateSamples(size_t sampleOffset)
	{
		MathUtils::generateHaltonSequence(NUMBER_OF_RANDOM_SAMPLES, NUMBER_OF_RANDOM_SAMPLE_DIMENSIONS,  m_randomSamples.getData()->samples, sampleOffset);
		m_randomSamples.flush();
	}

	void RaytraceCommonResources::updateSampledWavelengths(size_t sampleOffset)
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

	void RaytraceCommonResources::initSubpixelJitterSamples()
	{
		MathUtils::generateHaltonSequence(NUMBER_OF_SUBPIXEL_JITTER_SAMPLES, m_subpixelJitterSamples, 0);
		//[0,1] -> [-0.5, 0.5], ie offsets from pixel center
		for (size_t i = 0; i < NUMBER_OF_SUBPIXEL_JITTER_SAMPLES; ++i)
		{
			m_subpixelJitterSamples[i] = (m_subpixelJitterSamples[i] - vec2p(0.5f, 0.5f));
		}

	}

}