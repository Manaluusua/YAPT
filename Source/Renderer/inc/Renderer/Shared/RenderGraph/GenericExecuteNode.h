#pragma once
#include "CustomNode.h"
#include <Renderer/Shared/GfxTypes.h>

namespace YAPT
{
	class RenderGraph;
	class GenericExecuteNode : public CustomNode
	{
		friend class RenderGraph;
	public:


	protected:
		GenericExecuteNode(const char* name, RenderGraph* graph, size_t numberOfConnectionSlots, const RenderGraphNodeSlotDefinition* slotDefinitions);
		virtual ~GenericExecuteNode();
	};
}
