#pragma once

#include <Renderer/Shared/RenderPipeline/RenderPipeline.h>


namespace YAPT
{
	class RenderPipelineManager
	{
	public:
		void initialize(const RenderPipeline::InitializeContext& cntx);
		void shutdown();
		JobHandle prepare(const RenderPipeline::PrepareContext& cntx);
		JobHandle update(const RenderPipeline::UpdateContext& cntx);
		void execute(const RenderPipeline::ExecuteContext& cntx);

	private:

		void activatePipeline(RenderPipeline* p);
		void deactivatePipeline(RenderPipeline* p);

		void checkPipelineChange();

		std::vector<RenderPipeline*> m_pipelines;
		RenderPipeline* m_activePipeline;
		CRenderer* m_renderer;
		int32_t m_lastSelectedRenderPipelineIndex;
	};
	
}