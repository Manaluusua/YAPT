#pragma once

#include <Renderer/Shared/RenderGraph/RenderNode.h>
#include <Renderer/Vk/PlatformtypesVk.h>
namespace YAPT
{
	class RenderGraphVk;
	class RenderNodeVk final : public RenderNode
	{
		friend class RenderGraphVk;
	public:
		virtual RenderPassHandle getRenderPassHandle() final;

		

	private:
		RenderNodeVk(const char* name, RenderGraph* graph, size_t numberOfConnectionSlots, const RenderGraphNodeSlotDefinition* slotDefinitions);
		virtual ~RenderNodeVk();

		void setRenderPassAndIndex(VkRenderPass renderPass, size_t renderPassIndex)
		{
			m_renderPassHandle.index = (uint32_t)renderPassIndex;
			m_renderPassHandle.pass = renderPass;
		}
		
		RenderPassHandleVk m_renderPassHandle;
	};
}
