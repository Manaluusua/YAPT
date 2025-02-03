#include <Renderer/Shared/RenderPipeline/SwapChainStage.h>
#include <Gfx/RenderGraph/RenderGraph.h>
namespace YAPT
{
	SwapChainStage::SwapChainStage()
		:m_swapChainNode(nullptr)
	{

	}
	SwapChainStage::~SwapChainStage()
	{

	}

	void SwapChainStage::initialize()
	{
		m_swapChainNode = getGraph()->createSwapChainNode();
	}
	void SwapChainStage::shutdown()
	{

	}
	void SwapChainStage::onRenderResolutionChanged(const RenderResolutionDependantResourcesData& data)
	{

	}
	void SwapChainStage::prepare(const PrepareData& data)
	{
		m_swapChainNode->setSwapChain(data.swapChain);

		RenderGraphResourceId swapChainResourceId = m_swapChainNode->getRenderGraphResourceIdForSlot(0);

		TextureHandle tex = Gfx::getTextureHandleToNextBackbuffer(data.swapChain);
		m_swapChainNode->getGraph()->setRenderGraphResourceTexture(swapChainResourceId, tex);
	}
	void SwapChainStage::update(const UpdateData& data)
	{

	}

	RenderStageConnection SwapChainStage::getOutputConnection(size_t id)
	{
		RenderStageConnection conn{nullptr, size_t(-1)};
		assert(!"Swapchain stage has no outputs");
		return conn;
	}
	void SwapChainStage::setInputConnection(size_t id, const RenderStageConnection& connection)
	{
		switch (id)
		{
		case SWAPCHAIN_STAGE_CONNECTION_COLOR:
		{
			m_swapChainNode->getGraph()->createEdge(connection.node, connection.slot, m_swapChainNode, 0);
			break;
		}
		default:
			assert(false && "Stage connection id not recognized");
		}
	}
}