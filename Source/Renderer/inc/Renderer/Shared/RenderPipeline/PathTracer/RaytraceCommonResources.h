#pragma once

#include <Math/Math.h>
#include <Gfx/GfxTypes.h>
#include <spectralConstants.h>
#include <lightTypes.h>
#include <Renderer/Shared/RenderPipeline/PathTracer/PathIntegratorSubStage.h>
#include <Renderer/Shared/Utility/PostProcessUtility.h>
#include <Renderer/Shared/CRenderer.h>
#include <array>
#include <vector>

namespace YAPT
{
	constexpr uint32_t NUMBER_OF_RANDOM_SAMPLES = 1;
	constexpr uint32_t NUMBER_OF_RANDOM_SAMPLE_DIMENSIONS = 256; //keep in sync with shader
	constexpr size_t ENVIRONMENT_TYPE_NONE = 0;
	constexpr size_t ENVIRONMENT_TYPE_CUBE = 1;
	constexpr size_t ENVIRONMENT_TYPE_LONGLAT = 2;

	constexpr uint32_t CONSTANTS_FLAG_DISABLE_TEXEL_JITTER = YAPTBIT(1);

	struct RaytraceConstantData
	{
		mat4p uvToView;
		mat4p viewToUV;
		mat4p worldToView;
		mat4p viewToWorld;
		vec4p worldBoundsMin;
		vec4p worldBoundsMax;
		vec4p cameraPosition;
		vec4p targetTexDimensions;
		vec4p targetOffsetScaleBias;
		vec2p rayUVOffset;
		uint32_t maxRayDepth;
		uint32_t envTextureIndex;
		uint32_t envType;
		float envIntensityScale;
		uint32_t lightCount;
		uint32_t flags;
		uint32_t regularizeAfterVertices;
		uint32_t punctualLightCount;
		uint32_t pad1;
		uint32_t pad2;
		vec4p lensParams; //x: aperture radius, y: focus distance, z: view space forward z sign, w: unused
	};

	struct RandomSamples
	{
		uint32_t samples[NUMBER_OF_RANDOM_SAMPLES * NUMBER_OF_RANDOM_SAMPLE_DIMENSIONS];
	};

	class RenderGraph;
	class BindlessMaterialManager;
	class BindlessMeshManager;
	struct PunctualLightParameters;

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
			uint32_t lightType;
			vec4p positionCosInner;
			vec4p directionCosOuter;
			vec4p intensity;
			float selectionPDF;
			float aliasThreshold;
			uint32_t aliasIndex;
			float punctualSelectionPDF;
			float punctualAliasThreshold;
			uint32_t punctualAliasIndex;
			uint32_t pad0;
			uint32_t pad1;
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
			JobSystem* prepareTasksPool;

		};

		struct  UpdateParams
		{
			JobSystem* updateTasksPool;
			size_t sampleOffset;
			size_t spectralSampleOffset;
			uvec4p targetOffsetScaleBias;
			uvec2p raysPerFrame;
			uvec2p renderResolution;
		};


		RaytraceCommonResources();
		~RaytraceCommonResources();

		static void setupCommonSamplers(CRenderer* rend, PipelineLayoutHelper& helper, const ShaderPipelineReflection& refl, ShaderModuleType module);

		void initialize(CRenderer* rend, RenderResourcesPool* resourcesPool, BindlessMaterialManager* matMngr, BindlessMeshManager* meshMngr);
		void shutdown();
		void updateCommonResourcesToDescriptorSet(DescriptorSetHandle handle);
		JobHandle prepare(const RaytraceCommonResources::PrepareParams& params);
		JobHandle update(const RaytraceCommonResources::UpdateParams& params);

		virtual void sceneChanged(const PathIntegratorSubStage::SceneData& sceneData) final;

	private:
		constexpr static uint32_t COMBINE_BOUNDS_MAX_JOBS = 8;

		struct CombineBoundsJobItem
		{
			const AABB* objectBounds;
			size_t boundsOffset;
			size_t boundsCount;
			AABB combinedBounds;
		};

		void updateSamples(size_t sampleOffset);
		JobHandle setupLightDataJob(JobSystem* threadPool);
		void uploadLightData();
		float estimateMeshLightPower(size_t meshLightIndex) const;
		float estimatePunctualLightPower(const PunctualLightParameters& light) const;
		JobHandle setupWorldBoundsJob(JobSystem* threadPool);


		CRenderer* m_renderer;
		BindlessMaterialManager* m_materialMngr;
		BindlessMeshManager* m_meshMngr;
		FixedSizeGpuBufferHelper<RaytraceConstantData> m_rayTraceConstants;
		FixedSizeGpuBufferHelper<RandomSamples> m_randomSamples;
		DynamicSizeGpuBufferHelper<uvec2p> m_renderObjectMaterialAndMeshIndices;
		DynamicSizeGpuBufferHelper<RenderObjectTransformDataGPU> m_renderObjectTransformData;
		DynamicSizeGpuBufferHelper<LightEntryGPU> m_lightDataGPU;
		DynamicSizeGpuBufferHelper<float> m_instanceLightSelectionPDFs;
		std::vector<size_t> m_instanceOffsetPerRenderObject;
		size_t m_instanceCount = 0;

		std::vector<float> m_lightSelectionPDFs;
		std::vector<float> m_lightAliasThresholds;
		std::vector<uint32_t> m_lightAliasIndices;
		std::vector<float> m_punctualSelectionPDFs;
		std::vector<float> m_punctualAliasThresholds;
		std::vector<uint32_t> m_punctualAliasIndices;

		std::array<CombineBoundsJobItem, COMBINE_BOUNDS_MAX_JOBS> m_combineBoundsJobs;
		AABB m_worldBounds;
	};

}