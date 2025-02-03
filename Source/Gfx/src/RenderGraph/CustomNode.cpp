#include <Gfx/RenderGraph/CustomNode.h>

namespace YAPT
{
	CustomNode::CustomNode(const char* name, RenderGraph* graph, size_t numberOfConnectionSlots, const RenderGraphNodeSlotDefinition* slotDefinitions)
		:RenderGraphNode(RenderGraphNode::Type::CUSTOM, name, graph, numberOfConnectionSlots, slotDefinitions)
	{

	}
	CustomNode::~CustomNode()
	{

	}
}