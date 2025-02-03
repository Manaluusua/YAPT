#include <Gfx/RenderGraph/GenericExecuteNode.h>

namespace YAPT
{
	GenericExecuteNode::GenericExecuteNode(const char* name, RenderGraph* graph, size_t numberOfConnectionSlots, const RenderGraphNodeSlotDefinition* slotDefinitions)
		:CustomNode(name, graph, numberOfConnectionSlots, slotDefinitions)
	{

	}
	GenericExecuteNode::~GenericExecuteNode()
	{

	}
}