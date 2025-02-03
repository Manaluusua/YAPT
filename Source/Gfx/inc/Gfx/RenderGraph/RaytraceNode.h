#ifndef YAPT_SHARED_RAYTRACENODE_H
#define YAPT_SHARED_RAYTRACENODE_H

#include "RenderGraphNode.h"
#include <Gfx/GfxTypes.h>

namespace YAPT
{


	class RaytraceNode : public RenderGraphNode
	{
	public:


	protected:
		RaytraceNode(const char* name, RenderGraph* graph, size_t numberOfConnectionSlots, const RenderGraphNodeSlotDefinition* slotDefinitions);
		virtual ~RaytraceNode();
	};
}

#endif


