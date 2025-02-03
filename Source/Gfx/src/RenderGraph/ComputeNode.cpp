#include <Gfx/RenderGraph/ComputeNode.h>

namespace YAPT
{
	ComputeNode::ComputeNode(const char* name, RenderGraph* graph, size_t numberOfConnectionSlots, const RenderGraphNodeSlotDefinition* slotDefinitions)
		:RenderGraphNode(RenderGraphNode::Type::COMPUTE, name, graph, numberOfConnectionSlots, slotDefinitions)
	{

	}
	ComputeNode::~ComputeNode()
	{

	}
}