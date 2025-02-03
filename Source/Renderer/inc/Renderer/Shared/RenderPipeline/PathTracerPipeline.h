#ifndef YAPT_SHARED_SIMPLERTRENDERPIPELINE_H
#define YAPT_SHARED_SIMPLERTRENDERPIPELINE_H

#include "RenderPipelineBase.h"
#include <Gfx/RenderGraph/RenderGraph.h>

namespace YAPT
{
	class PathTracerPipeline : public RenderPipelineBase
	{
	public:
		PathTracerPipeline();
		virtual ~PathTracerPipeline();

	protected:
		virtual void setupRenderPipeline() override;

		
	};
}
#endif