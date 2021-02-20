#include <Renderer/Vk/ComputeNodeVk.h>

namespace YAPT
{


	ComputeNodeVk::ComputeNodeVk(const char* name, RenderGraph* graph, size_t numberOfConnectionSlots, const RenderGraphNodeSlotDefinition* slotDefinitions)
		:ComputeNode(name, graph, numberOfConnectionSlots, slotDefinitions)
	{

	}

	ComputeNodeVk::~ComputeNodeVk()
	{

	}
	
}