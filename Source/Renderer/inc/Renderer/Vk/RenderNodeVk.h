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
		

	private:
		RenderNodeVk(const char* name, RenderGraph* graph, size_t numberOfConnectionSlots, const RenderGraphNodeSlotDefinition* slotDefinitions);
		virtual ~RenderNodeVk();

		void setRenderPassIndex(size_t renderPassIndex)
		{
			m_renderPassIndex = renderPassIndex;
		}

		size_t m_renderPassIndex;
	};
}
