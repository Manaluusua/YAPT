#pragma once

#include <Renderer/Shared/RenderPipeline/RenderStage.h>
#include <Gfx/RenderGraph/RenderNode.h>
#include <Renderer/Shared/Utility/PostProcessUtility.h>
#include <Renderer/Shared/Utility/ShaderTableHelper.h>
#include <Gfx/RenderGraph/RaytraceNode.h>
#include <spectralConstants.h>
#include <Renderer/Shared/CRenderer.h>
namespace YAPT
{
	constexpr uint32_t RAY_MAX_VOLUMES_ENTERED = 4;
	constexpr uint32_t NUMBER_OF_RANDOM_SAMPLES = 256;
	constexpr uint32_t NUMBER_OF_SUBPIXEL_JITTER_SAMPLES = 60;
	class RenderGraph;
	class PathIntegratorSubStage
	{
	public:

		class AccelerationStructureProvider
		{
		public:
			virtual TopLevelAccelerationStructureHandle getAccelerationStructure(CommandBufferHandle cmd) = 0;
		};

		struct UpdateParams
		{
			size_t sampleOffset;
			uvec2p rayGenOffsetInTexels;
			uvec2p raysPerFrame;;
		};

		PathIntegratorSubStage();
		~PathIntegratorSubStage();

		void initialize(AccelerationStructureProvider* accStructProvider, CRenderer* rend, RenderGraph* graph);
		void shutdown();
		void onRenderGraphCompiled(const RenderStage::RenderGraphLifetimeData& data);
		void onRenderResolutionChanged(const RenderStage::RenderResolutionDependantResourcesData& data, uvec2 newResolution);

		void update(const UpdateParams& params);

		void getOutput(RenderGraphNode** node, size_t& slotOut);

		void updateShaderTable(const RenderObjectId* ids, MaterialPerSubmeshArray* materials, MeshInternal** meshes, size_t* instanceOffsets, size_t objectCount, size_t instancesCount);

	private:

		struct RaytracePayload
		{
			float throughput[SPECTRAL_SAMPLES_COUNT];
			float absorption[SPECTRAL_SAMPLES_COUNT];
			float totalLight[SPECTRAL_SAMPLES_COUNT];
			float volumesEntered[RAY_MAX_VOLUMES_ENTERED];
			vec3p rayOrigin;
			uint32_t rayIndex;
			vec3p rayDirection;
			uint32_t pathLength;

			uint32_t numberVolumesEntered;
			uint32_t rayState;
			uint32_t spectralSampleSetIndex;
			uint32_t flags;
		};


		struct RaytraceConstantData
		{
			mat4p uvToView;
			mat4p viewToWorld;
			vec4p cameraPosition;
			vec2p rayUVOffset;
			uint32_t currentSampleIndex;
			uint32_t maxRayDepth;
		};

		struct RandomSamples
		{
			vec4p samples[NUMBER_OF_RANDOM_SAMPLES];
		};

		struct SpectralDataConstants
		{
			vec4 spdSampleLambda[(SPECTRAL_SAMPLES_COUNT * SPECTRAL_SAMPLESET_COUNT + 3) / 4];
			vec4 spdSamplePdf[(SPECTRAL_SAMPLES_COUNT * SPECTRAL_SAMPLESET_COUNT + 3) / 4];
			uint32_t sampleSetOffset;
		};

		struct RayHitShaderTableConstantData
		{
			uvec2p indexBuffer;
			uvec2p normalBuffer;
			uvec2p tangentBuffer;
			uvec2p uvBuffer;

			vec4p specAmountClearCoatAmountIORRoughness;
			vec4p albedoTransparency;
			vec4p specularMetalness;
			vec4p absorptionDielectricIOR;
			vec4p emissiveRoughness;

			float anisotropy;
			float anisotropyRotation;
			uint32_t materialMask;
			float thinFilmThickness;

			vec2p cauchysCoefficients;
			float sheenAmount;
			float pad0;


			vec4p sheenColorRoughness;
			uvec2p albedoTexIndexAndScale;
			uvec2p normalTexIndexAndScale;
			uvec2p ormTexIndexAndScale;
			uvec2p emissiveTexIndexAndScale;

		};


		struct RayMissShaderTableConstantData
		{
			uint32_t envTextureIndex;
			uint32_t envType;
		};	
		//stride and offset are assumed in dwords (uint32/float32) in the shader
		static uvec2p packBufferInfo(uint32_t bufferIndex, uint32_t bufferStride, uint32_t bufferOffset);
		void writeShaderTableEntryAndConstantData(RenderObjectId id, const MaterialPerSubmeshArray& mat, const MeshInternal* mesh, size_t submeshIndex, ShaderTableEntry* entry);

		void initSubpixelJitterSamples();

		void executeRaytrace(const RenderGraphNodeExecutionContext& exec);

		void updateSamples(size_t sampleOffset);
		void updateSampledWavelengths(size_t sampleOffset);

		RenderGraph* m_graph;
		CRenderer* m_renderer;
		AccelerationStructureProvider* m_accStructProvider;

		ShaderTableHelper m_shaderTableHelper;
		

		RaytraceNode* m_rtNode;
		PipelineLayoutHelper m_rtLayout;
		RaytracePipelineStateHandle m_raytracePso;
		DescriptorSetHandle m_rtDescSet;
		FixedSizeGpuBufferHelper<RaytraceConstantData> m_rayTraceConstants;
		FixedSizeGpuBufferHelper<RandomSamples> m_randomSamples;
		FixedSizeGpuBufferHelper<SpectralDataConstants> m_spectralDataConstants;

		vec2p m_subpixelJitterSamples[NUMBER_OF_SUBPIXEL_JITTER_SAMPLES];
		bool m_applySubpixelJitter;

		uvec2 m_renderResolution;
		UpdateParams m_lastUpdateParams;
	};
}
