#pragma once

#include <Renderer/Shared/RenderGraph/RaytraceNode.h>

namespace YAPT
{
	class RenderGraphDx12;
	class RaytraceNodeDx12 final : public RaytraceNode
	{
		friend class RenderGraphDx12;
	public:

	private:
		RaytraceNodeDx12(const char* name, RenderGraph* graph, size_t numberOfConnectionSlots, const RenderGraphNodeSlotDefinition* slotDefinitions);
		virtual ~RaytraceNodeDx12();
	};
}
