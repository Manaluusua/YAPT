#pragma once

#include <Renderer/Shared/RenderPipeline/RenderPipeline.h>
#include <Renderer/ReadbackHandle.h>
#include <vector>

namespace YAPT
{
	class RenderPipelineManager
	{
	public:
		RenderPipelineManager();

		void initialize(const RenderPipeline::InitializeContext& cntx);
		void shutdown();
		JobHandle prepare(const RenderPipeline::PrepareContext& cntx);
		JobHandle update(const RenderPipeline::UpdateContext& cntx);
		void execute(const RenderPipeline::ExecuteContext& cntx);

		void readbackRequested(ReadbackTarget target);
		TextureHandle getReadbackTextureResource(ReadbackTarget target);
		BufferHandle getReadbackBufferResource(ReadbackTarget target);

	private:

		struct ActivePipeline
		{
			RenderPipeline* pipeline;
			int32_t priority;
			RenderPipeline::ExecuteOutput output;
		};

		struct ExecuteJobData
		{
			RenderPipeline* pipeline;
			const RenderPipeline::ExecuteContext* cntx;
			RenderPipeline::ExecuteOutput* output;
		};

		void activatePipeline(RenderPipeline* p, int32_t priority);
		void deactivatePipeline(RenderPipeline* p);

		void checkPipelineChange();

		JobHandle combineCollectedJobHandles(JobSystem* jobSystem);

		std::vector<RenderPipeline*> m_pipelines;
		std::vector<ActivePipeline> m_activePipelines;
		std::vector<ExecuteJobData> m_executeJobData;
		std::vector<JobHandle> m_jobHandles;
		RenderPipeline* m_mainPipeline;
		CRenderer* m_renderer;
		int32_t m_lastSelectedRenderPipelineIndex;
	};

}
