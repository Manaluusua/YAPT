#ifndef YAPT_SHARED_TONEMAPSTAGE_H
#define YAPT_SHARED_TONEMAPSTAGE_H

#include <Renderer/Shared/RenderPipeline/RenderStage.h>
#include <Renderer/Shared/RenderGraph/RenderNode.h>
#include <Renderer/Shared/RenderGraph/ComputeNode.h>
#include <Renderer/Shared/RenderPipeline/RenderStageUtilities.h>
#include <Renderer/Shared/Utility/PostProcessUtility.h>

namespace YAPT
{
	class TonemapStage final : public RenderStage
	{
	public:

		enum TonemapStageConnection
		{
			TONEMAP_STAGE_CONNECTION_COLOR
		};
		TonemapStage(bool outputDirectlyToSwapchain);
		~TonemapStage();

		virtual void initialize() final;
		virtual void shutdown() final;
		virtual void onRenderGraphCompiled(const RenderGraphLifetimeData& data) final;
		virtual void onRenderResolutionChanged(const RenderResolutionDependantResourcesData& data) final;
		virtual void prepare(const PrepareData& data) final;
		virtual void update(const UpdateData& data) final;

		virtual RenderStageConnection getOutputConnection(size_t id) final;
		virtual void setInputConnection(size_t id, const RenderStageConnection& connection) final;

	private:

		void replicateRenderVars();

		struct histogramConstants
		{
			uvec2p resolution;
			float minEV;
			float pad0;
		};


		struct PrepareExposureInfoData
		{
			vec4p unused;
		};


		struct TonemapConstants
		{
			histogramConstants histogramConstants;
			PrepareExposureInfoData prepareTonemapDataConstants;
			vec4p toeMidShoulderInit;
			vec4p exposureEyeAdaptTime;
		};
		
		struct ExposureInfo
		{
			float exposureMultiplier;
		};

		struct LuminanceHistogramAnalysisResults
		{
			float averageLuminance;
		};

		 
		void executeGenerateHistogram(const RenderGraphNodeExecutionContext& execContext);
		void executeAnalyzeHistogram(const RenderGraphNodeExecutionContext& execContext);
		void executePrepareTonemapData(const RenderGraphNodeExecutionContext& execContext);
		void executeTonemapping(const RenderGraphNodeExecutionContext& execContext);

		DeferredRenderGraphBindingUtility m_bindingsUtility;
		
		FixedSizeGpuBufferHelper<TonemapConstants> m_tonemapConstants;

		ComputeNode* m_generateHistogramNode;
		PostProcessComputePassUtility m_clearHistogramPass;
		PostProcessComputePassUtility m_generateHistogramPass;

		ComputeNode* m_analyzeHistogramNode;
		PostProcessComputePassUtility m_analyzeHistogramPass;

		ComputeNode* m_preparetonemapDataNode;
		PostProcessComputePassUtility m_preparetonemapDataPass;

		RenderNode* m_tonemapNode;
		PostProcessGraphicsPassUtility m_tonemapPass;
		

		TextureHandle m_colorTarget;
		BufferHandle m_histogramBuffer;
		BufferHandle m_luminanceAnalysisResultsBuffer;
		BufferHandle m_exposureInfo;

		uint32_t m_renderWidth;
		uint32_t m_renderHeight;

		bool m_outputToSwapchain;
		bool m_isFirstRun;
	};
}
#endif