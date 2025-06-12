#pragma once

#include <Math/Math.h>
#include <Gfx/GfxTypes.h>
#include <spectralConstants.h>
#include <Renderer/Shared/RenderPipeline/PathTracer/RaytraceCommonResources.h>
#include <Renderer/Shared/Utility/PostProcessUtility.h>
#include <Renderer/Shared/CRenderer.h>
namespace YAPT
{
	constexpr uint32_t NUMBER_OF_RANDOM_SAMPLES = 256;
	constexpr uint32_t NUMBER_OF_RANDOM_SAMPLE_DIMENSIONS = 8; //keep in sync with shader
	constexpr uint32_t NUMBER_OF_SUBPIXEL_JITTER_SAMPLES = 60;
	constexpr size_t ENVIRONMENT_TYPE_NONE = 0;
	constexpr size_t ENVIRONMENT_TYPE_CUBE = 1;
	constexpr size_t ENVIRONMENT_TYPE_LONGLAT = 2;

	struct RaytraceConstantData
	{
		mat4p uvToView;
		mat4p viewToWorld;
		vec4p cameraPosition;
		vec4p targetTexDimensions;
		vec2p rayUVOffset;
		uint32_t currentSampleIndex;
		uint32_t maxRayDepth;
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

	class AccelerationStructureProvider
	{
	public:
		virtual void prepareAccelerationStructure() = 0;
		virtual TopLevelAccelerationStructureHandle getAccelerationStructure() = 0;

		virtual ~AccelerationStructureProvider(){}
	};


	class RenderGraph;
	class BindlessMaterialManager;
	class BindlessMeshManager;

	class RaytraceCommonResources
	{
	public:

		struct  UpdateParams
		{
			size_t sampleOffset;
			size_t spectralSampleOffset;
			uvec2p rayGenOffsetInTexels;
			uvec2p raysPerFrame;
			uvec2p renderResolution;
		};

		static void setupCommonSamplers(CRenderer* rend, PipelineLayoutHelper& helper, const ShaderPipelineReflection& refl, ShaderModuleType module);

		void initialize(CRenderer* rend, RenderResourcesPool* resourcesPool, BindlessMaterialManager* matMngr, BindlessMeshManager* meshMngr);
		void shutdown();
		void updateCommonResourcesToDescriptorSet(DescriptorSetHandle handle);
		void update(const RaytraceCommonResources::UpdateParams& params);

		

	private:
		void initSubpixelJitterSamples();
		void updateSamples(size_t sampleOffset);
		void updateSampledWavelengths(size_t sampleOffset);

		CRenderer* m_renderer;
		BindlessMaterialManager* m_materialMngr;
		BindlessMeshManager* m_meshMngr;
		FixedSizeGpuBufferHelper<RaytraceConstantData> m_rayTraceConstants;
		FixedSizeGpuBufferHelper<RandomSamples> m_randomSamples;
		FixedSizeGpuBufferHelper<SpectralDataConstants> m_spectralDataConstants;

		vec2p m_subpixelJitterSamples[NUMBER_OF_SUBPIXEL_JITTER_SAMPLES];
		bool m_applySubpixelJitter;

	};

}