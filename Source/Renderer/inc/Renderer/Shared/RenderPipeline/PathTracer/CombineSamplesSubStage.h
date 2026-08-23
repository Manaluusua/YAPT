#pragma once

#include <Renderer/Shared/RenderPipeline/RenderStage.h>
#include <Gfx/RenderGraph/RenderNode.h>
#include <Renderer/Shared/Utility/PostProcessUtility.h>

namespace YAPT
{

	class RenderGraph;
	class CombineSamplesSubStage 
	{
	public:

		enum class OutputResource
		{
			COLOR
		};

		enum class InputResource
		{
			COLOR,
			MATERIAL_PARAMS0,
			MATERIAL_PARAMS1
		};

		struct UpdateParams
		{
			uvec4p targetOffsetScaleBias;
			uvec2p sourceTextureResolution;
			uvec2p targetTextureResolution;
			uint64_t samplesPerPixel;
			bool clearAccumulated;
			
		};

		CombineSamplesSubStage();
		~CombineSamplesSubStage();

		 void initialize(CRenderer* rend, RenderGraph* graph);
		 void shutdown();
		 void onRenderGraphCompiled(const RenderStage::RenderGraphLifetimeData& data);
		 void onRenderResolutionChanged(const RenderStage::RenderResolutionDependantResourcesData& data, uvec2 newResolution);

		 void update(const UpdateParams& params);

		 void setInput(InputResource resource, RenderGraphNode* node, size_t slot);
		 void getOutput(OutputResource outputResource, RenderGraphNode** node, size_t& slotOut);

	private:

		struct MergeNewSamplesParams
		{
			uvec4p targetTextureOffsetScaleBias;
			uvec4p randomSequence;
			uvec2p targetTextureDimensions;
			uvec2p sourceTextureDimensions;
			uint64_t sampleCount;
			uint32_t accumulationTargetCount;
			float fireflyClampScale;
			float fireflyClampRelaxPower;
			uint32_t pad0;
			uint32_t pad1;
			uint32_t pad2;
		};

		struct DenoiseParams
		{
			mat4p uvToView;
			mat4p viewToWorld;
			vec4p cameraPositionWS;
			vec4p textureDimensions;
			uvec4p modePassIndex;
			vec4p bilaterWeightParams;
			uvec4p miscParams;
		};

		struct ClearAccumulatedSamplesParams
		{
			uvec4p targetTextureDimensions;
			vec4p clearValue;
			uint32_t accumulationTargetCount;
		};

		struct DenoisePass
		{
			ComputeNode* m_denoiseNode;
			PostProcessComputePassUtility m_denoisePassUtility;
			CombineSamplesSubStage* subStage;
			FixedSizeGpuBufferHelper<DenoiseParams> m_denoiseConstants;
			uint32_t passIndex;
		};

		void executeClear(const RenderGraphNodeExecutionContext& exec);
		void executeMergeToPrevious(const RenderGraphNodeExecutionContext& exec);
		void executeDenoise(DenoisePass& pass, const RenderGraphNodeExecutionContext& exec);

		void precalculateKernelWeights(uint32_t width, float tau, float* dataPtr);

		RenderGraph* m_graph;
		CRenderer* m_renderer;

		std::vector<DenoisePass> m_denoisePasses;

		ComputeNode* m_clearNode;
		ComputeNode* m_mergeNode;
		
		PostProcessComputePassUtility m_clearMergeBufferPass;
		PostProcessComputePassUtility m_mergePass;
		
		FixedSizeGpuBufferHelper<ClearAccumulatedSamplesParams> m_clearMergeBufferConstants;
		FixedSizeGpuBufferHelper<MergeNewSamplesParams> m_mergeSamplesConstants;
		UpdateParams m_lastUpdateParams;
		uvec2 m_renderResolution;
	};
}
