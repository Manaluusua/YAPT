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
	constexpr uint32_t NUMBER_OF_RANDOM_SAMPLES = 1;
	constexpr uint32_t NUMBER_OF_RANDOM_SAMPLE_DIMENSIONS = 256; //keep in sync with shader
	constexpr size_t ENVIRONMENT_TYPE_NONE = 0;
	constexpr size_t ENVIRONMENT_TYPE_CUBE = 1;
	constexpr size_t ENVIRONMENT_TYPE_LONGLAT = 2;

	constexpr uint32_t CONSTANTS_FLAG_WRITE_FIRST_BOUNCE_MATERIAL_PARAMS = YAPTBIT(0);
	constexpr uint32_t CONSTANTS_FLAG_DISABLE_TEXEL_JITTER = YAPTBIT(1);

	struct RaytraceConstantData
	{
		mat4p uvToView;
		mat4p viewToWorld;
		vec4p worldBoundsMin;
		vec4p worldBoundsMax;
		vec4p cameraPosition;
		vec4p targetTexDimensions;
		vec2p rayUVOffset;
		uint32_t maxRayDepth;
		uint32_t envTextureIndex;
		uint32_t envType;
		uint32_t lightCount;
		uint32_t flags;
	};

	struct RandomSamples
	{
		uint32_t samples[NUMBER_OF_RANDOM_SAMPLES * NUMBER_OF_RANDOM_SAMPLE_DIMENSIONS];
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

		void updateSamples(size_t sampleOffset);
		void setupLightDataJob(ThreadPool* threadPool);
		void setupWorldBoundsJob(ThreadPool* threadPool);


		CRenderer* m_renderer;
		BindlessMaterialManager* m_materialMngr;
		BindlessMeshManager* m_meshMngr;
		FixedSizeGpuBufferHelper<RaytraceConstantData> m_rayTraceConstants;
		FixedSizeGpuBufferHelper<RandomSamples> m_randomSamples;
		DynamicSizeGpuBufferHelper<uvec2p> m_renderObjectMaterialAndMeshIndices;
		DynamicSizeGpuBufferHelper<RenderObjectTransformDataGPU> m_renderObjectTransformData;
		DynamicSizeGpuBufferHelper<LightEntryGPU> m_lightDataGPU;
		std::vector<size_t> m_instanceOffsetPerRenderObject;

		std::array<CombineBoundsJobItem, 8> m_combineBoundsJobs;
	};

}