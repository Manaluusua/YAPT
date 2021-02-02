#pragma once

#include <Renderer/Shared/RenderGraph/ComputeNode.h>

namespace YAPT
{
	class RenderGraphDx12;
	class ComputeNodeDx12 final: public ComputeNode
	{
		friend class RenderGraphDx12;
	public:
		

	private:
		ComputeNodeDx12(const char* name, RenderGraph* graph, size_t numberOfConnectionSlots, const RenderGraphNodeSlotDefinition* slotDefinitions);
		virtual ~ComputeNodeDx12();
	};
}
