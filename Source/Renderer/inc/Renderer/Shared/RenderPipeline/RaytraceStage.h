#ifndef YAPT_SHARED_RAYTRACESTAGE_H
#define YAPT_SHARED_RAYTRACESTAGE_H

#include <Renderer/Shared/RenderPipeline/RenderStage.h>
#include <Renderer/Shared/RenderGraph/RaytraceNode.h>
#include <Renderer/Shared/RenderGraph/RenderNode.h>
#include <Renderer/Shared/RenderGraph/GenericExecuteNode.h>
#include <Renderer/Shared/RenderPipeline/RenderStageUtilities.h>
#include <Renderer/Shared/Utility/PostProcessUtility.h>
#include <Renderer/Shared/Utility/AccelerationStructureHelper.h>
#include <Renderer/Shared/Utility/ShaderTableHelper.h>

namespace YAPT
{
	constexpr uint32_t NUMBER_OF_RANDOM_SAMPLES = 256;
	constexpr uint32_t NUMBER_OF_SUBPIXEL_JITTER_SAMPLES = 60;
	constexpr uint32_t RAY_MAX_VOLUMES_ENTERED = 4;
	class RaytraceStage final : public RenderStage
	{
	public:
		 
		enum RaytraceStageConnection
		{
			RAYTRACE_STAGE_CONNECTION_COLOR
		};
		 
		RaytraceStage();
		~RaytraceStage();

		virtual void initialize() final;
		virtual void shutdown() final;
		virtual void onRenderGraphCompiled(const RenderGraphLifetimeData& data) final;
		virtual void onRenderResolutionChanged(const RenderResolutionDependantResourcesData& data) final;
		virtual void prepare(const PrepareData& data) final;
		virtual void update(const UpdateData& data) final;

		virtual RenderStageConnection getOutputConnection(size_t id) final;
		virtual void setInputConnection(size_t id, const RenderStageConnection& connection) final;

		
		void setRayTraceResolutionReductionFactor(uint32_t factor); //0 fullres, 1 is dimensions/2^1, 2 is dimensions/2^2 etc 

	private:
		       
		bool hasCameraMoved();
		bool hasSceneChanged();

		void clearAccumulatedFrames();

		void updateEffectiveRaytraceResolution();

		void updateAccumulatedFrames();

		glm::uvec4 getCurrentResolveTargetTexelOffsetParams();
		glm::vec2 getCurrentRayGenerationOffset();

		uint32_t getCurrentNumberOfSamplesPerPixel();

		bool doesShaderTableNeedUpdate();
		void updateShaderTable();
		void writeShaderTableEntry(RenderObjectId id, const MaterialInternal* mat, const MeshInternal* mesh, ShaderTableEntry* entry);

		void initSubpixelJitterSamples();

		static glm::uvec2 packBufferInfo(uint32_t bufferIndex, uint32_t bufferStride, uint32_t bufferOffset);

		struct RaytracePayload
		{
			glm::vec3 coeff;
			uint32_t rayTerminated;
			glm::vec3 throughput;
			uint32_t pathLength;
			glm::vec3 rayOrigin;
			uint32_t rayIndex;
			glm::vec3 rayDirection;
			uint32_t numberVolumesEntered;
			float volumesEntered[RAY_MAX_VOLUMES_ENTERED];
			glm::vec3 absorption;
		};


		struct RaytraceConstantData
		{
			glm::mat4 uvToView;
			glm::mat4 viewToWorld;
			glm::vec4 cameraPosition;
			glm::vec2 rayUVOffset;
			uint32_t currentSampleIndex;
			uint32_t maxRayDepth;
		};
		  
		struct RandomSamples
		{
			glm::vec4 samples[NUMBER_OF_RANDOM_SAMPLES];
		};
		 
		
		struct RayHitShaderTableConstantData
		{
			glm::uvec2 indexBuffer;
			glm::uvec2 normalBuffer;
			glm::uvec2 tangentBuffer;
			glm::uvec2 uvBuffer;

			glm::vec4 specAmountClearCoatAmountIORRoughness;
			glm::vec4 albedoTransparency;
			glm::vec4 specularMetalness;
			glm::vec4 absorptionDielectricIOR;
			glm::vec4 emissiveRoughness;

			float anisotropy;
			float anisotropyRotation;
			uint32_t materialMask;
			float thinFilmThickness;

			float sheenAmount;
			float pad0;
			float pad1;
			float pad2;

			glm::vec4 sheenColorRoughness;
			uint32_t albedoTexIndex;
			uint32_t normalTexIndex;
			uint32_t ormTexIndex;
			uint32_t emissiveTexIndex;

		};


		struct RayMissShaderTableConstantData
		{
			uint32_t cubeMapIndex;
		};

		void initRaytracePass(const RenderGraphLifetimeData& data);
		void setupMergePass(RenderResourcesPool* pool);

		void executeRaytrace(const RenderGraphNodeExecutionContext& exec);
		void executeMergeToPrevious(const RenderGraphNodeExecutionContext& exec);

		void updateRandomSamples();

		AccelerationStructureHelper m_accStructureHelper;
		ShaderTableHelper m_shaderTableHelper;

		RaytraceNode* m_rtNode;
		PipelineLayoutHelper m_rtLayout;
		RaytracePipelineStateHandle m_raytracePso;
		DescriptorSetHandle m_rtDescSet;
		FixedSizeGpuBufferHelper<RaytraceConstantData> m_rayTraceConstants;
		FixedSizeGpuBufferHelper<RandomSamples> m_randomSamples;

		ComputeNode* m_mergeNode;
		PostProcessComputePassUtility m_clearMergeBufferPass;
		PostProcessComputePassUtility m_mergePass;
		FixedSizeGpuBufferHelper<ClearAccumulatedSamplesParams> m_clearMergeBufferConstants;;
		FixedSizeGpuBufferHelper<MergeNewSamplesParams> m_mergeSamplesConstants;;

		uint32_t m_raysPerFrameWidth;
		uint32_t m_raysPerFrameHeight;

		uint32_t m_resolveTargetWidth;
		uint32_t m_resolveTargetHeight;

		uint32_t m_framesAccumulated;
		uint32_t m_raysPerFrameDivisor;

		DeferredRenderGraphBindingUtility m_bindingsUtility;

		glm::vec2 m_subpixelJitterSamples[NUMBER_OF_SUBPIXEL_JITTER_SAMPLES];

		bool m_applySubpixelJitter;

	};
}
#endif