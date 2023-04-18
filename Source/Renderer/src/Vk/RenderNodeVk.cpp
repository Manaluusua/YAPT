#include <Renderer/Vk/RenderNodeVk.h>

namespace YAPT
{



	RenderNodeVk::RenderNodeVk(const char* name, RenderGraph* graph, size_t numberOfConnectionSlots, const RenderGraphNodeSlotDefinition* slotDefinitions)
		:RenderNode(name, graph, numberOfConnectionSlots, slotDefinitions)
	{
		

	}
	RenderNodeVk::~RenderNodeVk()
	{

	}

	RenderPassHandle RenderNodeVk::getRenderPassHandle()
	{
		return &m_renderPassHandle;
	}

}