#include <Gfx/RenderGraph/ComputeNode.h>

namespace YAPT
{
	ComputeNode::ComputeNode(const char* name, RenderGraph* graph, size_t numberOfConnectionSlots, const RenderGraphNodeSlotDefinition* slotDefinitions)
		:RenderGraphNode(RenderGraphNode::Type::COMPUTE, name, graph, numberOfConnectionSlots, slotDefinitions)
	{
		for (size_t i = 0; i < numberOfConnectionSlots; ++i)
		{
			assert((slotDefinitions[i].resourceDescription.shaderStages & SHADERSTAGE_COMPUTE) != 0);
		}
	}
	ComputeNode::~ComputeNode()
	{

	}
}