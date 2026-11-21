#include <Renderer/Shared/RenderPipeline/Realtime/RealtimePipeline.h>
#include <Renderer/Shared/RenderPipeline/Realtime/DeferredMaterialsStage.h>
#include <Renderer/Shared/RenderPipeline/Realtime/VisibilityDebugResolveStage.h>
#include <Renderer/Shared/RenderPipeline/CopyTextureStage.h>
#include <Renderer/Shared/RenderPipeline/SwapChainStage.h>

namespace YAPT
{
	RealtimePipeline::RealtimePipeline()
		:m_sceneResources(nullptr),
		m_deferredMaterialsStage(nullptr),
		m_resolveStage(nullptr),
		m_copyStage(nullptr),
		m_swapChainStage(nullptr)
	{

	}
	RealtimePipeline::~RealtimePipeline()
	{

	}

	void RealtimePipeline::initialize(const InitializeContext& cntx)
	{
		m_matManager.init(cntx.renderer);
		m_meshManager.init(cntx.renderer);

		RenderPipelineBase::initialize(cntx);
	}

	void RealtimePipeline::shutdown()
	{
		RenderPipelineBase::shutdown();

		m_sceneResources->shutdown();
		delete m_sceneResources;
		m_sceneResources = nullptr;

		m_matManager.shutdown();
		m_meshManager.shutdown();
	}

	void RealtimePipeline::createStages()
	{
		m_sceneResources = new RealtimeSceneResources();
		m_sceneResources->init(m_renderer, m_pipelineLifetimeResources, &m_matManager, &m_meshManager);

		m_deferredMaterialsStage = new DeferredMaterialsStage(m_sceneResources);
		m_resolveStage = new VisibilityDebugResolveStage();
		m_copyStage = new CopyTextureStage(ResourceFormat::UNKNOWN, ResourceFormat::RGBA8_SRGB);
		m_swapChainStage = new SwapChainStage;

		addRenderStage(m_deferredMaterialsStage);
		addRenderStage(m_resolveStage);
		addRenderStage(m_copyStage);
		addRenderStage(m_swapChainStage);

		m_numberOfCommandBuffers = 1;
	}

	void RealtimePipeline::connectStages()
	{
		m_resolveStage->setInputConnection(VisibilityDebugResolveStage::VISIBILITYRESOLVE_STAGE_CONNECTION_VISIBILITY, m_deferredMaterialsStage->getOutputConnection(DeferredMaterialsStage::DEFERREDMATERIALS_STAGE_CONNECTION_VISIBILITY));
		m_copyStage->setInputConnection(CopyTextureStage::COPYTEXTURE_STAGE_CONNECTION_SOURCE, m_resolveStage->getOutputConnection(VisibilityDebugResolveStage::VISIBILITYRESOLVE_STAGE_CONNECTION_COLOR));
		m_swapChainStage->setInputConnection(SwapChainStage::SWAPCHAIN_STAGE_CONNECTION_COLOR, m_copyStage->getOutputConnection(CopyTextureStage::COPYTEXTURE_STAGE_CONNECTION_DESTINATION));
	}

	void RealtimePipeline::afterRenderGraphCompiled()
	{
		RenderStageConnection conn = m_resolveStage->getOutputConnection(VisibilityDebugResolveStage::VISIBILITYRESOLVE_STAGE_CONNECTION_COLOR);
		RenderGraphResourceId resId = getRenderGraph()->getRenderGraphResourceIdUsedInSlot(conn.node->getSortedIndex(), conn.slot);
		getRenderGraph()->markExtraUsageFlagsForResource(resId, RESOURCE_USAGE_COPY_SOURCE);

		setNamedResourceMapping(NamedRenderPipelineTexture::FinalColor, conn.node->getSortedIndex(), conn.slot);
	}

	JobHandle RealtimePipeline::update(const UpdateContext& cntx)
	{
		m_matManager.update();
		m_meshManager.update();
		m_sceneResources->update(uvec2((uint32_t)m_renderResolutionWidth, (uint32_t)m_renderResolutionHeight));
		return RenderPipelineBase::update(cntx);
	}
}
