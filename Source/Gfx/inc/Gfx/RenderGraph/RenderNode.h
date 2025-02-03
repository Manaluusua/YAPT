#ifndef YAPT_SHARED_RENDERNODE_H
#define YAPT_SHARED_RENDERNODE_H

#include "RenderGraphNode.h"
#include <Gfx/GfxTypes.h>
namespace YAPT
{
	class RenderNode : public RenderGraphNode
	{
	public:
		virtual RenderPassHandle getRenderPassHandle() = 0;
		

		size_t getNumberOfColorTargets() const;
		bool hasDepthStencil() const;

		size_t getNodeSlotIndexForRenderTargetIndex(size_t renderTargetIndex) const;
		size_t getNodeSlotIndexForDepthStencil() const;

	protected:
		RenderNode(const char* name, RenderGraph* graph, size_t numberOfConnectionSlots, const RenderGraphNodeSlotDefinition* slotDefinitions);
		virtual ~RenderNode();

		std::vector<size_t> m_renderTargetSlotIndices;
		size_t m_depthStencilSlot;
	};
}

#endif