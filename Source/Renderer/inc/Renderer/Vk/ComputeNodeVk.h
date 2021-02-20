#pragma once

#include <Renderer/Shared/RenderGraph/ComputeNode.h>

namespace YAPT
{
	class RenderGraphVk;
	class ComputeNodeVk final: public ComputeNode
	{
		friend class RenderGraphVk;
	public:
		

	private:
		ComputeNodeVk(const char* name, RenderGraph* graph, size_t numberOfConnectionSlots, const RenderGraphNodeSlotDefinition* slotDefinitions);
		virtual ~ComputeNodeVk();
	};
}
