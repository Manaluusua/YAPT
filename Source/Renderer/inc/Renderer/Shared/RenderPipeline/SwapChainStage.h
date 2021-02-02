#pragma once

#include <Renderer/Shared/RenderPipeline/RenderStage.h>
#include <Renderer/Shared/RenderGraph/RenderNode.h>


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

		virtual void initialize() final;
		virtual void shutdown() final;
		virtual void onRenderResolutionChanged(const RenderResolutionDependantResourcesData& data) final;
		virtual void prepare(const PrepareData& data) final;
		virtual void update(const UpdateData& data) final;

		virtual RenderStageConnection getOutputConnection(size_t id) final;
		virtual void setInputConnection(size_t id, const RenderStageConnection& connection) final;

	private:


		SwapChainNode* m_swapChainNode;
	};
}
