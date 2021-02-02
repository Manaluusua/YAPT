#include <Renderer/Shared/RenderPipeline/DebugDrawPipeline.h>
#include <Renderer/Shared/RenderPipeline/MeshDebugStage.h>
#include <Renderer/Shared/RenderPipeline/SwapChainStage.h>

namespace YAPT
{

	DebugDrawPipeline::DebugDrawPipeline()
	{

	}
	DebugDrawPipeline::~DebugDrawPipeline()
	{

	}


	void DebugDrawPipeline::setupRenderPipeline()
	{
		MeshDebugStage* meshDbg = new MeshDebugStage(true);
		SwapChainStage* scStage = new SwapChainStage;

		addRenderStage(meshDbg);
		addRenderStage(scStage);


		scStage->setInputConnection(SwapChainStage::SWAPCHAIN_STAGE_CONNECTION_COLOR, meshDbg->getOutputConnection(MeshDebugStage::MESHDEBUG_STAGE_CONNECTION_COLOR));

		getRenderGraph()->compile();

		setupCommandBufferPool(1);
		setupRenderGraphSchedulingGroups(1);
	}


}