#ifndef YAPT_SHARED_COMPUTENODE_H
#define YAPT_SHARED_COMPUTENODE_H

#include "RenderGraphNode.h"
#include <Gfx/GfxTypes.h>

namespace YAPT
{
	class ComputeNode : public RenderGraphNode
	{
	public:


	protected:
		ComputeNode(const char* name, RenderGraph* graph, size_t numberOfConnectionSlots, const RenderGraphNodeSlotDefinition* slotDefinitions);
		virtual ~ComputeNode();
	};
}

#endif