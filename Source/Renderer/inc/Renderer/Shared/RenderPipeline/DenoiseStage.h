#ifndef YAPT_SHARED_DENOISESTAGE_H
#define YAPT_SHARED_DENOISESTAGE_H

#include <Renderer/Shared/RenderPipeline/RenderStage.h>
#include <Gfx/RenderGraph/RenderNode.h>
#include <Gfx/RenderGraph/ComputeNode.h>
#include <Renderer/Shared/Utility/PostProcessUtility.h>

namespace YAPT
{
	class DenoiseStage final : public RenderStage
	{
	public:

		enum DenoiseStageConnection
		{
			TONEMAP_STAGE_CONNECTION_COLOR
		};
		DenoiseStage();
		~DenoiseStage();

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

	};
}
#endif