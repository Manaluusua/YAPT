#pragma once

#include <Renderer/Shared/RenderPipeline/RenderStage.h>
#include <Gfx/RenderGraph/RenderNode.h>

namespace YAPT
{

	class RenderGraph;
	class GBufferStage final : public RenderStage
	{
	public:

		enum GBufferSubStageConnection
		{
			GBUFFER_STAGE_CONNECTION_ALBEDOALPHA,
			GBUFFER_STAGE_CONNECTION_NORMALROUGHNESS,
			GBUFFER_STAGE_CONNECTION_DEPTH
		};
		GBufferStage();
		~GBufferStage();

		virtual void initialize() final;
		virtual void shutdown() final;
		virtual void onRenderGraphCompiled(const RenderGraphLifetimeData& data) final;
		virtual void onRenderResolutionChanged(const RenderResolutionDependantResourcesData& data) final;
		virtual void prepare(const PrepareData& data) final;
		virtual void update(const UpdateData& data) final;

		virtual RenderStageConnection getOutputConnection(size_t id) final;
		virtual void setInputConnection(size_t id, const RenderStageConnection& connection) final;

	private:

		RenderGraph* m_graph;
		CRenderer* m_renderer;
	};
}
