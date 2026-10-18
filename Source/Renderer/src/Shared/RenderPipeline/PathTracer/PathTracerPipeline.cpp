#include <Renderer/Shared/RenderPipeline/PathTracer/PathTracerPipeline.h>
#include <Renderer/Shared/RenderPipeline/PathTracer/PathTraceStage.h>
#include <Renderer/Shared/RenderPipeline/TonemapStage.h>
#include <Renderer/Shared/RenderPipeline/CopyTextureStage.h>
#include <Renderer/Shared/RenderPipeline/SwapChainStage.h>

namespace YAPT
{

	PathTracerPipeline::PathTracerPipeline(PathTraceStage::PathIntegratorType type)
		:m_pathTraceType(type),
		m_pathTraceStage(nullptr),
		m_tonemapStage(nullptr),
		m_copyStage(nullptr),
		m_swapChainStage(nullptr)
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


	void PathTracerPipeline::createStages()
	{
		m_pathTraceStage = new PathTraceStage(&m_matManager, &m_meshManager, m_pathTraceType);
		m_tonemapStage = new TonemapStage(false);
		m_copyStage = new CopyTextureStage(ResourceFormat::UNKNOWN, ResourceFormat::RGBA8_SRGB);
		m_swapChainStage = new SwapChainStage;

		addRenderStage(m_pathTraceStage);
		addRenderStage(m_tonemapStage);
		addRenderStage(m_copyStage);
		addRenderStage(m_swapChainStage);

		m_numberOfCommandBuffers = 2;
	}

	void PathTracerPipeline::connectStages()
	{
		m_tonemapStage->setInputConnection(TonemapStage::TONEMAP_STAGE_CONNECTION_COLOR, m_pathTraceStage->getOutputConnection(PathTraceStage::RAYTRACE_STAGE_CONNECTION_COLOR));
		m_copyStage->setInputConnection(CopyTextureStage::COPYTEXTURE_STAGE_CONNECTION_SOURCE, m_tonemapStage->getOutputConnection(TonemapStage::TONEMAP_STAGE_CONNECTION_COLOR));
		m_swapChainStage->setInputConnection(SwapChainStage::SWAPCHAIN_STAGE_CONNECTION_COLOR, m_copyStage->getOutputConnection(CopyTextureStage::COPYTEXTURE_STAGE_CONNECTION_DESTINATION));
	}

	void PathTracerPipeline::afterRenderGraphCompiled()
	{
		//for readback
		RenderStageConnection conn = m_tonemapStage->getOutputConnection(TonemapStage::TONEMAP_STAGE_CONNECTION_COLOR);
		RenderGraphResourceId resId = getRenderGraph()->getRenderGraphResourceIdUsedInSlot(conn.node->getSortedIndex(), conn.slot);
		getRenderGraph()->markExtraUsageFlagsForResource(resId, RESOURCE_USAGE_COPY_SOURCE);

		setNamedResourceMapping(NamedRenderPipelineTexture::FinalColor, conn.node->getSortedIndex(), conn.slot);
	}

	JobHandle PathTracerPipeline::update(const UpdateContext& cntx)
	{
		m_matManager.update();
		m_meshManager.update();
		return RenderPipelineBase::update(cntx);
	}
}