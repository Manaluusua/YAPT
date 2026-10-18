#include <Gfx/RenderGraph/RenderNode.h>

namespace YAPT
{
	RenderNode::RenderNode(const char* name, RenderGraph* graph, size_t numberOfConnectionSlots, const RenderGraphNodeSlotDefinition* slotDefinitions)
		:RenderGraphNode(RenderGraphNode::Type::RENDER, name, graph, numberOfConnectionSlots, slotDefinitions),
		m_renderTargetSlotIndices(nullptr),
		m_numberOfRenderTargets(0),
		m_depthStencilSlot(INVALID_NODESLOT_INDEX)
	{
		for (size_t i = 0; i < numberOfConnectionSlots; ++i)
		{
			if (slotDefinitions[i].resourceDescription.resourceUsage & RESOURCE_USAGE_RENDER_TARGET_TEXTURE)
			{
				++m_numberOfRenderTargets;
			}
		}

		m_renderTargetSlotIndices = getArena().allocateArray<size_t>(m_numberOfRenderTargets);

		size_t renderTargetIndex = 0;
		for (size_t i = 0; i < numberOfConnectionSlots; ++i)
		{
			if (slotDefinitions[i].resourceDescription.resourceUsage & RESOURCE_USAGE_RENDER_TARGET_TEXTURE)
			{
				m_renderTargetSlotIndices[renderTargetIndex++] = i;
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
		return m_numberOfRenderTargets;
	}
	bool RenderNode::hasDepthStencil() const
	{
		return m_depthStencilSlot != INVALID_NODESLOT_INDEX;
	}

	size_t RenderNode::getNodeSlotIndexForRenderTargetIndex(size_t renderTargetIndex) const
	{
		if (renderTargetIndex >= m_numberOfRenderTargets)
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

	uint64_t RenderNode::getAttachmentSignature() const
	{
		uint64_t hash = 14695981039346656037ull;
		auto mix = [&hash](uint64_t v)
		{
			hash ^= v;
			hash *= 1099511628211ull;
		};

		mix(m_numberOfRenderTargets);
		for (size_t i = 0; i < m_numberOfRenderTargets; ++i)
		{
			mix((uint64_t)m_slotDefinitions[m_renderTargetSlotIndices[i]].resourceDescription.resourceFormat);
		}

		mix(hasDepthStencil() ? 1 : 0);
		if (hasDepthStencil())
		{
			mix((uint64_t)m_slotDefinitions[m_depthStencilSlot].resourceDescription.resourceFormat);
		}

		return hash;
	}
}
