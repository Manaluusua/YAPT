#ifndef YAPT_SHARED_DEBUGDRAWPIPELINE_H
#define YAPT_SHARED_DEBUGDRAWPIPELINE_H

#include "RenderPipelineBase.h"
#include <Renderer/Shared/RenderGraph/RenderGraph.h>

namespace YAPT
{
	class DebugDrawPipeline : public RenderPipelineBase
	{
	public:
		DebugDrawPipeline();
		virtual ~DebugDrawPipeline();

	protected:
		virtual void setupRenderPipeline() override;


	};
}
#endif