#include <Gfx/RenderGraph/RenderNode.h>

namespace YAPT
{
	RenderNode::RenderNode(const char* name, RenderGraph* graph, size_t numberOfConnectionSlots, const RenderGraphNodeSlotDefinition* slotDefinitions)
		:RenderGraphNode(RenderGraphNode::Type::RENDER, name, graph, numberOfConnectionSlots, slotDefinitions),
		m_depthStencilSlot(INVALID_NODESLOT_INDEX)
	{

		for (size_t i = 0; i < numberOfConnectionSlots; ++i)
		{
			if (slotDefinitions[i].resourceDescription.resourceUsage & RESOURCE_USAGE_RENDER_TARGET_TEXTURE)
			{
				m_renderTargetSlotIndices.push_back(i);
			}
			if (slotDefinitions[i].resourceDescription.resourceUsage & RESOURCE_USAGE_DEPTH_STENCIL_TEXTURE)
			{
				assert(m_depthStencilSlot == INVALID_NODESLOT_INDEX);
				m_depthStencilSlot = i;
			}
		}



	}
	RenderNode::~RenderNode()
	{

	}


	size_t RenderNode::getNumberOfColorTargets() const
	{
		return m_renderTargetSlotIndices.size();
	}
	bool RenderNode::hasDepthStencil() const
	{
		return m_depthStencilSlot != INVALID_NODESLOT_INDEX;
	}

	size_t RenderNode::getNodeSlotIndexForRenderTargetIndex(size_t renderTargetIndex) const
	{
		if (renderTargetIndex >= m_renderTargetSlotIndices.size())
		{
			YAPT_LOG_ERROR("Asked for colortarget index %d from node %s, but node only has %d colortargets", renderTargetIndex, m_name, getNumberOfColorTargets());
			return INVALID_NODESLOT_INDEX;
		}
		return m_renderTargetSlotIndices[renderTargetIndex];
	}
	size_t RenderNode::getNodeSlotIndexForDepthStencil() const
	{
		return m_depthStencilSlot;
	}
}