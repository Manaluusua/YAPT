#pragma once

#include <Renderer/Shared/RenderPipeline/RenderStage.h>
#include <Gfx/RenderGraph/RenderNode.h>


namespace YAPT
{
	class SwapChainNode;
	class SwapChainStage : public RenderStage
	{
	public:

		enum SwapChainStageConnection
		{
			SWAPCHAIN_STAGE_CONNECTION_COLOR
		};
		SwapChainStage();
		~SwapChainStage();

		virtual void initialize() override;
		virtual void shutdown() override;
		virtual void onRenderResolutionChanged(const RenderResolutionDependantResourcesData& data) override;
		virtual JobHandle prepare(const PrepareData& data) override;
		virtual JobHandle update(const UpdateData& data) override;
		virtual void beforeExecute() override {};

		virtual RenderStageConnection getOutputConnection(size_t id) override;
		virtual void setInputConnection(size_t id, const RenderStageConnection& connection) override;

	private:


		SwapChainNode* m_swapChainNode;
	};
}
