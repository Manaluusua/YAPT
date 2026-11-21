#pragma once

#include <Renderer/Shared/RenderPipeline/RenderPipelineBase.h>
#include <Renderer/Shared/BindlessMaterialManager.h>
#include <Renderer/Shared/BindlessMeshManager.h>
#include <Renderer/Shared/RenderPipeline/Realtime/RealtimeSceneResources.h>

namespace YAPT
{
	class DeferredMaterialsStage;
	class VisibilityDebugResolveStage;
	class CopyTextureStage;
	class SwapChainStage;

	class RealtimePipeline : public RenderPipelineBase
	{
	public:
		RealtimePipeline();
		virtual ~RealtimePipeline();
		virtual void initialize(const InitializeContext& cntx) final;
		virtual void shutdown() final;
		virtual JobHandle update(const UpdateContext& cntx) final;

	protected:
		virtual void createStages() override;
		virtual void connectStages() override;
		virtual void afterRenderGraphCompiled() override;

		BindlessMaterialManager m_matManager;
		BindlessMeshManager m_meshManager;
		RealtimeSceneResources* m_sceneResources;

		DeferredMaterialsStage* m_deferredMaterialsStage;
		VisibilityDebugResolveStage* m_resolveStage;
		CopyTextureStage* m_copyStage;
		SwapChainStage* m_swapChainStage;
	};
}
