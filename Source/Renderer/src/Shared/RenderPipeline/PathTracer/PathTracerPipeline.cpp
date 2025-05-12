#include <Renderer/Shared/RenderPipeline/PathTracer/PathTracerPipeline.h>
#include <Renderer/Shared/RenderPipeline/PathTracer/PathTraceStage.h>
#include <Renderer/Shared/RenderPipeline/TonemapStage.h>
#include <Renderer/Shared/RenderPipeline/SwapChainStage.h>

namespace YAPT
{

	PathTracerPipeline::PathTracerPipeline(PathTraceStage::PathIntegratorType type)
		:m_pathTraceType(type)
	{

	}
	PathTracerPipeline::~PathTracerPipeline()
	{

	}

	void PathTracerPipeline::initialize(const InitializeContext& cntx)
	{
		m_matManager.init(cntx.renderer);
		m_meshManager.init(cntx.renderer);

		RenderPipelineBase::initialize(cntx);
	}
	void PathTracerPipeline::shutdown()
	{
		m_matManager.shutdown();
		m_meshManager.shutdown();

		RenderPipelineBase::shutdown();
	}


	void PathTracerPipeline::setupRenderPipeline()
	{
		PathTraceStage* rtStage = new PathTraceStage(&m_matManager, &m_meshManager, m_pathTraceType);
		TonemapStage* tstage = new TonemapStage(true);
		SwapChainStage* scStage = new SwapChainStage;

		addRenderStage(rtStage);
		addRenderStage(tstage);
		addRenderStage(scStage);
		

		tstage->setInputConnection(TonemapStage::TONEMAP_STAGE_CONNECTION_COLOR, rtStage->getOutputConnection(PathTraceStage::RAYTRACE_STAGE_CONNECTION_COLOR));
		scStage->setInputConnection(SwapChainStage::SWAPCHAIN_STAGE_CONNECTION_COLOR, tstage->getOutputConnection(TonemapStage::TONEMAP_STAGE_CONNECTION_COLOR));
		
		getRenderGraph()->compile();
		getRenderGraph()->setupScheduling(2);
	}

	void PathTracerPipeline::update(const UpdateContext& cntx)
	{
		m_matManager.update();
		m_meshManager.update();
		RenderPipelineBase::update(cntx);
	}


}