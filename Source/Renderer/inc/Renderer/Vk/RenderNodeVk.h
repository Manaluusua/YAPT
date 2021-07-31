#pragma once

#include <Renderer/Shared/RenderGraph/RenderNode.h>

namespace YAPT
{
	class RenderGraphVk;
	class RenderNodeVk final : public RenderNode
	{
		friend class RenderGraphVk;
	public:
		virtual RenderPassHandle getRenderPassHandle() final;

		size_t getRenderPassIndex() { return m_renderPassIndex; }
		size_t getSubPassIndex() { return m_subPassIndex; }
		

	private:
		RenderNodeVk(const char* name, RenderGraph* graph, size_t numberOfConnectionSlots, const RenderGraphNodeSlotDefinition* slotDefinitions);
		virtual ~RenderNodeVk();

		void setupRenderPass(size_t renderPassIndex, size_t subPassIndex)
		{
			m_renderPassIndex = renderPassIndex;
			m_subPassIndex = subPassIndex;
		}

		size_t m_renderPassIndex;
		size_t m_subPassIndex;
	};
}
