#pragma once
#include "CustomNode.h"
#include <Renderer/Shared/GfxApi.h>

namespace YAPT
{

	class RenderGraph;
	class SwapChainNode : public CustomNode
	{
		friend class RenderGraph;
	public:
		void setSwapChain(SwapChainHandle swapChain) { m_swapChain = swapChain; }
		virtual void renderGraphAfterSubmit();
		
	protected:

		SwapChainNode(const char* name, RenderGraph* graph);
		
		virtual ~SwapChainNode() 
		{
		}

		SwapChainHandle m_swapChain;

	};

}