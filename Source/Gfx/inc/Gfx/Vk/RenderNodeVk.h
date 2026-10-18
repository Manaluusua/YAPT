#pragma once

#include <Gfx/RenderGraph/RenderNode.h>
#include <Gfx/Vk/PlatformtypesVk.h>
namespace YAPT
{
	class RenderGraphVk;
	class RenderNodeVk final : public RenderNode
	{
		friend class RenderGraphVk;
	public:
		virtual RenderPassHandle getRenderPassHandle() final;
		uint32_t getRenderGraphInternalRenderNodeResourcesIndex() const { return m_renderGraphResourcesIndex; }
		const VkClearValue* getClearValues() const { return m_clearValues; }
		const uint32_t getClearValueCount() const { return m_clearValueCount; }

	private:
		RenderNodeVk(const char* name, RenderGraph* graph, size_t numberOfConnectionSlots, const RenderGraphNodeSlotDefinition* slotDefinitions);
		virtual ~RenderNodeVk();

		void setRenderPass(VkRenderPass renderPass, uint32_t subpassIndex)
		{
			m_renderPassHandle.pass = renderPass;
			m_renderPassHandle.index = subpassIndex;
		}
		
		void setRenderGraphResourcesIndex(uint32_t index)
		{
			m_renderGraphResourcesIndex = index;
		}

		RenderPassHandleVk m_renderPassHandle;
		uint32_t m_renderGraphResourcesIndex;
		VkClearValue* m_clearValues;
		uint32_t m_clearValueCount;
	};
}
