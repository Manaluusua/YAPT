#include <Gfx/Vk/RaytraceNodeVk.h>

namespace YAPT
{


	RaytraceNodeVk::RaytraceNodeVk(const char* name, RenderGraph* graph, size_t numberOfConnectionSlots, const RenderGraphNodeSlotDefinition* slotDefinitions)
		:RaytraceNode(name, graph, numberOfConnectionSlots, slotDefinitions)
	{

	}
	RaytraceNodeVk::~RaytraceNodeVk()
	{

	}

}