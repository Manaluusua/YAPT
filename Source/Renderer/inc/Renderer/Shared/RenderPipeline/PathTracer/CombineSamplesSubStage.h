#pragma once

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

		struct UpdateParams
		{
			uvec4p targetOffsetScaleBias;
			uvec2p sourceTextureResolution;
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

		 void setInput(RenderGraphNode* node, size_t slot);
		 void getOutput(RenderGraphNode** node, size_t& slotOut);

	private:

		struct MergeNewSamplesParams
		{
			uvec4p targetTextureOffsetScaleBias;
			uvec2p sourceTextureDimensions;
			uint64_t sampleCount;
		};

		struct ClearAccumulatedSamplesParams
		{
			uvec4p targetTextureDimensions;
			vec4p clearValue;
		};

		void executeMergeToPrevious(const RenderGraphNodeExecutionContext& exec);



		RenderGraph* m_graph;
		CRenderer* m_renderer;

		ComputeNode* m_mergeNode;
		PostProcessComputePassUtility m_clearMergeBufferPass;
		PostProcessComputePassUtility m_mergePass;
		FixedSizeGpuBufferHelper<ClearAccumulatedSamplesParams> m_clearMergeBufferConstants;
		FixedSizeGpuBufferHelper<MergeNewSamplesParams> m_mergeSamplesConstants;
		UpdateParams m_lastUpdateParams;
		uvec2 m_renderResolution;


	};
}
