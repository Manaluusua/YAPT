#ifndef YAPT_SHARED_RENDERPIPELINEBASE_H
#define YAPT_SHARED_RENDERPIPELINEBASE_H

#include <Renderer/Shared/RenderPipeline/RenderPipeline.h>
#include <Renderer/Shared/RenderGraph/RenderGraph.h>
#include <vector>

namespace YAPT
{
	class RenderResourcesPool;
	class RenderPipelineBase : public RenderPipeline
	{
	public:
		RenderPipelineBase();
		virtual ~RenderPipelineBase();

		virtual void initialize(const InitializeContext& cntx) override;
		virtual void shutdown() override;
		virtual void prepare(const PrepareContext& cntx) override;
		virtual void update(const UpdateContext& cntx) override;
		virtual void execute() override;

		RenderGraph* getRenderGraph() { return m_graph; }
		GfxApiHandle getGfxApiHandle() const { return m_gfxHandle; }

	protected:

		struct RenderNodeSequence
		{
			size_t offset;
			size_t count;
		};

		struct ScheduledRenderNodesPerBuffer
		{
			std::vector<RenderNodeSequence> renderNodeSequence;
		};

		void addRenderStage(RenderStage* stage);
		virtual void setupRenderPipeline() = 0;
		virtual void setupCommandBufferPool(size_t numberOfBuffersPerFrame);
		virtual void setupRenderGraphSchedulingGroups(size_t maximumNumberOfGroups);

		RenderGraph* m_graph;
		CRenderer* m_renderer;
		GfxApiHandle m_gfxHandle;
		
		CommandBufferPoolHandle m_cmdBufferPool;
		size_t m_numberOfCmdBuffersPerFrame;

		size_t m_renderResolutionWidth;
		size_t m_renderResolutionHeight;

		RenderResourcesPool* m_renderResolutionDependantResources;
		RenderResourcesPool* m_renderGraphLifetimeResources;

		std::vector<ScheduledRenderNodesPerBuffer> m_scheduledRenderGraphNodeGroups;

		std::vector<RenderStage*> m_stages;

		std::vector<CommandBufferHandle> m_commandBuffersRecording;

		bool m_firstPrepareAfterInit;

	};
}
#endif