#include <Gfx/Dx12/ComputeNodeDx12.h>

namespace YAPT
{


	ComputeNodeDx12::ComputeNodeDx12(const char* name, RenderGraph* graph, size_t numberOfConnectionSlots, const RenderGraphNodeSlotDefinition* slotDefinitions)
		:ComputeNode(name, graph, numberOfConnectionSlots, slotDefinitions)
	{

	}
	ComputeNodeDx12::~ComputeNodeDx12()
	{

	}
	
}