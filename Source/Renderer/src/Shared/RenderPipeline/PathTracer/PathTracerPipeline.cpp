#include <Renderer/Shared/RenderPipeline/PathTracer/PathTracerPipeline.h>
#include <Renderer/Shared/RenderPipeline/PathTracer/PathTraceStage.h>
#include <Renderer/Shared/RenderPipeline/TonemapStage.h>
#include <Renderer/Shared/RenderPipeline/CopyTextureStage.h>
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
		TonemapStage* tstage = new TonemapStage(false);
		CopyTextureStage* copyStage = new CopyTextureStage(ResourceFormat::UNKNOWN, ResourceFormat::RGBA8_SRGB);
		SwapChainStage* scStage = new SwapChainStage;

		addRenderStage(rtStage);
		addRenderStage(tstage);
		addRenderStage(copyStage);
		addRenderStage(scStage);
		

		tstage->setInputConnection(TonemapStage::TONEMAP_STAGE_CONNECTION_COLOR, rtStage->getOutputConnection(PathTraceStage::RAYTRACE_STAGE_CONNECTION_COLOR));
		copyStage->setInputConnection(CopyTextureStage::COPYTEXTURE_STAGE_CONNECTION_SOURCE, tstage->getOutputConnection(TonemapStage::TONEMAP_STAGE_CONNECTION_COLOR));
		scStage->setInputConnection(SwapChainStage::SWAPCHAIN_STAGE_CONNECTION_COLOR, copyStage->getOutputConnection(CopyTextureStage::COPYTEXTURE_STAGE_CONNECTION_DESTINATION));
		
		getRenderGraph()->compile();
		getRenderGraph()->setupScheduling(2);

		//for readback
		{
			RenderStageConnection conn = tstage->getOutputConnection(TonemapStage::TONEMAP_STAGE_CONNECTION_COLOR);
			RenderGraphResourceId resId = getRenderGraph()->getRenderGraphResourceIdUsedInSlot(conn.node->getSortedIndex(), conn.slot);
			getRenderGraph()->markExtraUsageFlagsForResource(resId, RESOURCE_USAGE_COPY_SOURCE);
		}
		
	}

	JobHandle PathTracerPipeline::update(const UpdateContext& cntx)
	{
		m_matManager.update();
		m_meshManager.update();
		return RenderPipelineBase::update(cntx);
	}

	TextureHandle PathTracerPipeline::getNamedResource(NamedRenderPipelineTexture tex)
	{
		return YAPT_NULL_HANDLE;
	}
	BufferHandle PathTracerPipeline::getNamedResource(NamedRenderPipelineBuffer buff)
	{
		return YAPT_NULL_HANDLE;
	}
}