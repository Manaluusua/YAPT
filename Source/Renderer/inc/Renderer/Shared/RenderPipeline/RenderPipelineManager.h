#pragma once

#include <Renderer/Shared/RenderPipeline/RenderPipeline.h>
#include <Renderer/ReadbackHandle.h>

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

		void readbackRequested(ReadbackTarget target);
		TextureHandle getReadbackTextureResource(ReadbackTarget target);
		BufferHandle getReadbackBufferResource(ReadbackTarget target);

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