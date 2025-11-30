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
			uvec2p targetTextureDimensions;
			uvec2p sourceTextureDimensions;
			uint64_t sampleCount;
		};

		struct DenoiseParams
		{
			vec4p textureDimensions;
			uvec4p denoiseMode;
		};

		struct ClearAccumulatedSamplesParams
		{
			uvec4p targetTextureDimensions;
			vec4p clearValue;
		};

		void executeClear(const RenderGraphNodeExecutionContext& exec);
		void executeMergeToPrevious(const RenderGraphNodeExecutionContext& exec);
		void executeDenoise(const RenderGraphNodeExecutionContext& exec);



		RenderGraph* m_graph;
		CRenderer* m_renderer;

		ComputeNode* m_clearNode;
		ComputeNode* m_mergeNode;
		ComputeNode* m_denoiseNode;
		PostProcessComputePassUtility m_clearMergeBufferPass;
		PostProcessComputePassUtility m_mergePass;
		PostProcessComputePassUtility m_denoisePass;
		FixedSizeGpuBufferHelper<ClearAccumulatedSamplesParams> m_clearMergeBufferConstants;
		FixedSizeGpuBufferHelper<MergeNewSamplesParams> m_mergeSamplesConstants;
		FixedSizeGpuBufferHelper<DenoiseParams> m_denoiseConstants;
		UpdateParams m_lastUpdateParams;
		uvec2 m_renderResolution;


	};
}
