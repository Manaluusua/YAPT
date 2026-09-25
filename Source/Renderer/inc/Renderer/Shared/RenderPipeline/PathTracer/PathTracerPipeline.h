#ifndef YAPT_SHARED_SIMPLERTRENDERPIPELINE_H
#define YAPT_SHARED_SIMPLERTRENDERPIPELINE_H

#include <Renderer/Shared/RenderPipeline/RenderPipelineBase.h>
#include <Gfx/RenderGraph/RenderGraph.h>
#include <Renderer/Shared/BindlessMaterialManager.h>
#include <Renderer/Shared/BindlessMeshManager.h>
#include <Renderer/Shared/RenderPipeline/PathTracer/PathTraceStage.h>


namespace YAPT
{
	class PathTracerPipeline : public RenderPipelineBase
	{
	public:
		PathTracerPipeline(PathTraceStage::PathIntegratorType type);
		virtual ~PathTracerPipeline();
		virtual void initialize(const InitializeContext& cntx) final;
		virtual void shutdown() final;
		virtual JobHandle update(const UpdateContext& cntx) final;

	protected:
		virtual void setupRenderPipeline() override;

		BindlessMaterialManager m_matManager;
		BindlessMeshManager m_meshManager;
		PathTraceStage::PathIntegratorType m_pathTraceType;
	};
}
#endif