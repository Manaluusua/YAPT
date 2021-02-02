#pragma once

#include <Renderer/Shared/RenderPipeline/RenderPipeline.h>


namespace YAPT
{
	class RenderPipelineManager
	{
	public:
		void initialize(const RenderPipeline::InitializeContext& cntx);
		void shutdown();
		void prepare(const RenderPipeline::PrepareContext& cntx);
		void update(const RenderPipeline::UpdateContext& cntx);
		void execute();

	private:

		void activatePipeline(RenderPipeline* p);
		void deactivatePipeline(RenderPipeline* p);

		std::vector<RenderPipeline*> m_pipelines;
		RenderPipeline* m_activePipeline;
		CRenderer* m_renderer;

	};
	
}