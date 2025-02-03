#include <Gfx/RenderGraph/RayTraceNode.h>

namespace YAPT
{
	RaytraceNode::RaytraceNode(const char* name, RenderGraph* graph, size_t numberOfConnectionSlots, const RenderGraphNodeSlotDefinition* slotDefinitions)
		:RenderGraphNode(RenderGraphNode::Type::RAYTRACE, name, graph, numberOfConnectionSlots, slotDefinitions)
	{

	}
	RaytraceNode::~RaytraceNode()
	{

	}
}