#ifndef YAPT_SHARED_RENDERPIPELINEBASE_H
#define YAPT_SHARED_RENDERPIPELINEBASE_H

#include <Renderer/Shared/RenderPipeline/RenderPipeline.h>
#include <Gfx/RenderGraph/RenderGraph.h>
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

		

		void addRenderStage(RenderStage* stage);
		virtual void setupRenderPipeline() = 0;

		RenderGraph* m_graph;
		CRenderer* m_renderer;
		GfxApiHandle m_gfxHandle;
		
		

		size_t m_renderResolutionWidth;
		size_t m_renderResolutionHeight;

		RenderResourcesPool* m_renderResolutionDependantResources;
		RenderResourcesPool* m_renderGraphLifetimeResources;

		std::vector<RenderStage*> m_stages;
		

		bool m_firstPrepareAfterInit;

	};
}
#endif