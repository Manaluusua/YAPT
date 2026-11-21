#pragma once

#include <Renderer/Shared/RenderPipeline/RenderStage.h>
#include <Gfx/RenderGraph/RenderNode.h>
#include <Renderer/Shared/Utility/PostProcessUtility.h>

namespace YAPT
{
	//colors the visibility target entries based on instance id, primitive id and uv
	class VisibilityDebugResolveStage final : public RenderStage
	{
	public:

		enum VisibilityDebugResolveStageConnection
		{
			VISIBILITYRESOLVE_STAGE_CONNECTION_COLOR,
			VISIBILITYRESOLVE_STAGE_CONNECTION_VISIBILITY
		};

		VisibilityDebugResolveStage();
		~VisibilityDebugResolveStage();

		virtual void initialize(const InitializeData& data) override;
		virtual void createNodes() override;
		virtual void shutdown() override;
		virtual void onRenderGraphCompiled(const RenderGraphLifetimeData& data) override;
		virtual void onRenderResolutionChanged(const RenderResolutionDependantResourcesData& data) override;
		virtual JobHandle prepare(const PrepareData& data) override;
		virtual JobHandle update(const UpdateData& data) override;
		virtual void beforeExecute(const ExecuteData& data) override {};

		virtual RenderStageConnection getOutputConnection(size_t id) override;
		virtual void setInputConnection(size_t id, const RenderStageConnection& connection) override;

	private:

		void execute(const RenderGraphNodeExecutionContext& execContext);

		PostProcessGraphicsPassUtility m_resolvePass;
		RenderNode* m_resolveNode;
		TextureHandle m_colorTarget;
	};
}
