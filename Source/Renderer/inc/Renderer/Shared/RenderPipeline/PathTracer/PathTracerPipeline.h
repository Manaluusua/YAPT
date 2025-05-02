#ifndef YAPT_SHARED_SIMPLERTRENDERPIPELINE_H
#define YAPT_SHARED_SIMPLERTRENDERPIPELINE_H

#include <Renderer/Shared/RenderPipeline/RenderPipelineBase.h>
#include <Gfx/RenderGraph/RenderGraph.h>
#include <Renderer/Shared/BindlessMaterialManager.h>
#include <Renderer/Shared/BindlessMeshManager.h>
namespace YAPT
{
	class PathTracerPipeline : public RenderPipelineBase
	{
	public:
		PathTracerPipeline();
		virtual ~PathTracerPipeline();
		virtual void initialize(const InitializeContext& cntx) final;
		virtual void shutdown() final;
		virtual void update(const UpdateContext& cntx) final;
	protected:
		virtual void setupRenderPipeline() override;

		BindlessMaterialManager m_matManager;
		BindlessMeshManager m_meshManager;
	};
}
#endif