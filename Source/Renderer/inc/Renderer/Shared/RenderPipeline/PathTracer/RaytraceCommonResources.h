#pragma once

#include <Math/Math.h>
#include <Gfx/GfxTypes.h>
#include <spectralConstants.h>
#include <Renderer/Shared/RenderPipeline/PathTracer/PathIntegratorSubStage.h>
#include <Renderer/Shared/Utility/PostProcessUtility.h>
#include <Renderer/Shared/CRenderer.h>
#include <array>

namespace YAPT
{
	constexpr uint32_t NUMBER_OF_RANDOM_SAMPLES_1D = 16;
	constexpr uint32_t NUMBER_OF_RANDOM_SAMPLES = (NUMBER_OF_RANDOM_SAMPLES_1D * NUMBER_OF_RANDOM_SAMPLES_1D);
	constexpr uint32_t NUMBER_OF_RANDOM_SAMPLE_DIMENSIONS = 8; //keep in sync with shader
	constexpr uint32_t NUMBER_OF_SUBPIXEL_JITTER_SAMPLES = 60;
	constexpr size_t ENVIRONMENT_TYPE_NONE = 0;
	constexpr size_t ENVIRONMENT_TYPE_CUBE = 1;
	constexpr size_t ENVIRONMENT_TYPE_LONGLAT = 2;

	struct RaytraceConstantData
	{
		mat4p uvToView;
		mat4p viewToWorld;
		vec4p worldBoundsMin;
		vec4p worldBoundsMax;
		vec4p cameraPosition;
		vec4p targetTexDimensions;
		vec2p rayUVOffset;
		uint32_t currentSampleIndex;
		uint32_t maxRayDepth;
		uint32_t envTextureIndex;
		uint32_t envType;
		uint32_t lightCount;
	};

	struct RandomSamples
	{
		float samples[NUMBER_OF_RANDOM_SAMPLES * NUMBER_OF_RANDOM_SAMPLE_DIMENSIONS];
	};

	struct SpectralDataConstants
	{
		vec4 spdSampleLambda[(SPECTRAL_SAMPLES_COUNT * SPECTRAL_SAMPLESET_COUNT + 3) / 4];
		vec4 spdSamplePdf[(SPECTRAL_SAMPLES_COUNT * SPECTRAL_SAMPLESET_COUNT + 3) / 4];
		uint32_t sampleSetOffset;
	};

	


	class RenderGraph;
	class BindlessMaterialManager;
	class BindlessMeshManager;

	class RaytraceCommonResources
	{
	public:

		struct LightEntryGPU
		{
			mat4p transform;
			mat4p transformInvTransp;
			vec4p centerRadius;
			uint32_t meshIndex;
			uint32_t matIndex;
			uint32_t instanceIndex;
			uint32_t pad0;
		};

		struct RenderObjectTransformDataGPU
		{
			vec4p objToWorldR0;
			vec4p objToWorldR1;
			vec4p objToWorldR2;
			vec4p worldToObjectR0;
			vec4p worldToObjectR1;
			vec4p worldToObjectR2;
		};

		struct PrepareParams
		{
			ThreadPool* prepareTasksPool;

		};

		struct  UpdateParams
		{
			ThreadPool* updateTasksPool;
			size_t sampleOffset;
			size_t spectralSampleOffset;
			uvec2p rayGenOffsetInTexels;
			uvec2p raysPerFrame;
			uvec2p renderResolution;
		};


		RaytraceCommonResources();
		~RaytraceCommonResources();

		static void setupCommonSamplers(CRenderer* rend, PipelineLayoutHelper& helper, const ShaderPipelineReflection& refl, ShaderModuleType module);

		void initialize(CRenderer* rend, RenderResourcesPool* resourcesPool, BindlessMaterialManager* matMngr, BindlessMeshManager* meshMngr);
		void shutdown();
		void updateCommonResourcesToDescriptorSet(DescriptorSetHandle handle);
		void prepare(const RaytraceCommonResources::PrepareParams& params);
		void update(const RaytraceCommonResources::UpdateParams& params);

		virtual void sceneChanged(const PathIntegratorSubStage::SceneData& sceneData) final;

	private:

		struct CombineBoundsJobItem
		{
			const AABB* objectBounds;
			size_t boundsOffset;
			size_t boundsCount;
			AABB combinedBounds;
		};

		void initSubpixelJitterSamples();
		void updateSamples(size_t sampleOffset);
		void updateSampledWavelengths(size_t sampleOffset);
		void setupLightDataJob(ThreadPool* threadPool);
		void setupWorldBoundsJob(ThreadPool* threadPool);


		CRenderer* m_renderer;
		BindlessMaterialManager* m_materialMngr;
		BindlessMeshManager* m_meshMngr;
		FixedSizeGpuBufferHelper<RaytraceConstantData> m_rayTraceConstants;
		FixedSizeGpuBufferHelper<RandomSamples> m_randomSamples;
		FixedSizeGpuBufferHelper<SpectralDataConstants> m_spectralDataConstants;
		DynamicSizeGpuBufferHelper<uvec2p> m_renderObjectMaterialAndMeshIndices;
		DynamicSizeGpuBufferHelper<RenderObjectTransformDataGPU> m_renderObjectTransformData;
		DynamicSizeGpuBufferHelper<LightEntryGPU> m_lightDataGPU;
		std::vector<size_t> m_instanceOffsetPerRenderObject;

		std::array<CombineBoundsJobItem, 8> m_combineBoundsJobs;

		vec2p m_subpixelJitterSamples[NUMBER_OF_SUBPIXEL_JITTER_SAMPLES];
		bool m_applySubpixelJitter;

	};

}