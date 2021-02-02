#pragma once

#include "RenderGraphNode.h"

namespace YAPT
{
	class CustomNode : public RenderGraphNode
	{
		friend class RenderGraph;
	protected:
		virtual void renderGraphExecutionBegin() {};
		virtual void renderGraphExecutionEnd() {};
		virtual void renderGraphAfterSubmit() {};

		CustomNode(const char* name, RenderGraph* graph, size_t numberOfConnectionSlots, const RenderGraphNodeSlotDefinition* slotDefinitions);
		virtual ~CustomNode();
	};

	
}
