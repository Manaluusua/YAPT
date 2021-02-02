#include <Renderer/Shared/RenderPipeline/PathTracerPipeline.h>
#include <Renderer/Shared/RenderPipeline/TonemapStage.h>
#include <Renderer/Shared/RenderPipeline/RaytraceStage.h>
#include <Renderer/Shared/RenderPipeline/SwapChainStage.h>

namespace YAPT
{

	PathTracerPipeline::PathTracerPipeline()
	{

	}
	PathTracerPipeline::~PathTracerPipeline()
	{

	}


	void PathTracerPipeline::setupRenderPipeline()
	{
		RaytraceStage *rtStage = new RaytraceStage;
		TonemapStage* tstage = new TonemapStage(true);
		SwapChainStage* scStage = new SwapChainStage;

		addRenderStage(rtStage);
		addRenderStage(tstage);
		addRenderStage(scStage);
		

		tstage->setInputConnection(TonemapStage::TONEMAP_STAGE_CONNECTION_COLOR, rtStage->getOutputConnection(RaytraceStage::RAYTRACE_STAGE_CONNECTION_COLOR));
		scStage->setInputConnection(SwapChainStage::SWAPCHAIN_STAGE_CONNECTION_COLOR, tstage->getOutputConnection(TonemapStage::TONEMAP_STAGE_CONNECTION_COLOR));
		
		getRenderGraph()->compile();

		setupCommandBufferPool(2);
		setupRenderGraphSchedulingGroups(2);
	}


}