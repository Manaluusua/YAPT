#ifndef YAPT_SHARED_DEBUGDRAWPIPELINE_H
#define YAPT_SHARED_DEBUGDRAWPIPELINE_H

#include "RenderPipelineBase.h"
#include <Gfx/RenderGraph/RenderGraph.h>
#include <Renderer/Shared/RenderPipeline/MeshDebugStage.h>
#include <Renderer/Shared/RenderPipeline/SwapChainStage.h>

namespace YAPT
{
	class DebugDrawPipeline : public RenderPipelineBase
	{
	public:
		DebugDrawPipeline();
		virtual ~DebugDrawPipeline();

	protected:
		virtual void createStages() override;
		virtual void connectStages() override;

		MeshDebugStage* m_meshDebugStage;
		SwapChainStage* m_swapChainStage;


	};
}
#endif