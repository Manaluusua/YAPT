#include <Gfx/RenderGraph/SwapChainNode.h>
#include <Gfx/RenderGraph/RenderGraph.h>


namespace YAPT
{

	

	SwapChainNode::SwapChainNode(const char* name, RenderGraph* graph, RenderGraphNodeSlotDefinition* definitions, size_t definitionCount)
		:CustomNode(name, graph, definitionCount, definitions),
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