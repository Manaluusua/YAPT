#include <Renderer/Shared/RenderGraph/SwapChainNode.h>
#include <Renderer/Shared/RenderGraph/RenderGraph.h>


namespace YAPT
{

	static const RenderGraphNodeSlotDefinition SWAPCHAINSLOTDEF = { ResourceDimension::TEXTURE_2D,
	ResourceFormat::RGBA8_SRGB,
	RESOURCE_USAGE_PRESENTABLE_TEXTURE,
	ACCESS_FLAGS_READ,
	SHADERSTAGE_NONE,
	1,
	1
	};

	SwapChainNode::SwapChainNode(const char* name, RenderGraph* graph)
		:CustomNode(name, graph, 1, &SWAPCHAINSLOTDEF),
		m_swapChain(YAPT_NULL_HANDLE)

	{

	}

	void SwapChainNode::renderGraphAfterSubmit()
	{
		if (m_swapChain != YAPT_NULL_HANDLE)
		{
			Gfx::present(m_graph->getGfxApiHandle(), m_swapChain);
		}
	}
}