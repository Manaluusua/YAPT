#pragma once

#include <Gfx/RenderGraph/RaytraceNode.h>

namespace YAPT
{
	class RenderGraphVk;
	class RaytraceNodeVk final : public RaytraceNode
	{
		friend class RenderGraphVk;
	public:

	private:
		RaytraceNodeVk(const char* name, RenderGraph* graph, size_t numberOfConnectionSlots, const RenderGraphNodeSlotDefinition* slotDefinitions);
		virtual ~RaytraceNodeVk();
	};
}
