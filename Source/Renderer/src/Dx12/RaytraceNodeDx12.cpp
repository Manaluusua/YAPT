#include <Renderer/Dx12/RaytraceNodeDx12.h>

namespace YAPT
{


	RaytraceNodeDx12::RaytraceNodeDx12(const char* name, RenderGraph* graph, size_t numberOfConnectionSlots, const RenderGraphNodeSlotDefinition* slotDefinitions)
		:RaytraceNode(name, graph, numberOfConnectionSlots, slotDefinitions)
	{

	}
	RaytraceNodeDx12::~RaytraceNodeDx12()
	{

	}

}