#include <Renderer/Dx12/RenderNodeDx12.h>
#include <Renderer/Dx12/YaptToDx12Conversions.h>

namespace YAPT
{



	RenderNodeDx12::RenderNodeDx12(const char* name, RenderGraph* graph, size_t numberOfConnectionSlots, const RenderGraphNodeSlotDefinition* slotDefinitions)
		:RenderNode(name, graph, numberOfConnectionSlots, slotDefinitions),
		m_rtvDescHeapOffset(0),
		m_dsvDescHeapOffset(0),
		m_rtvDescHeapSlot(0),
		m_dsvDescHeapSlot(0)
	{
		m_renderPassDecl.renderTargetCount = (UINT)getNumberOfColorTargets();

		for (size_t i = 0; i < m_renderPassDecl.renderTargetCount; ++i)
		{
			size_t slot = getNodeSlotIndexForRenderTargetIndex(i);
			m_renderPassDecl.rtvFormats[i] = yaptToDx12Format(slotDefinitions[slot].resourceDescription.resourceFormat);
		}

		if (hasDepthStencil())
		{
			size_t slot = getNodeSlotIndexForDepthStencil();
			m_renderPassDecl.dsvFormat = yaptToDx12Format(slotDefinitions[slot].resourceDescription.resourceFormat);
		}
		else
		{
			m_renderPassDecl.dsvFormat = DXGI_FORMAT_UNKNOWN;
		}

	}
	RenderNodeDx12::~RenderNodeDx12()
	{

	}

	RenderPassHandle RenderNodeDx12::getRenderPassHandle()
	{
		return &m_renderPassDecl;
	}

	void RenderNodeDx12::setRtvHeapDescriptorBaseOffset(size_t offset)
	{
		m_rtvDescHeapOffset = offset;
	}
	size_t RenderNodeDx12::getRtvHeapDescriptorBaseOffset() const
	{
		return m_rtvDescHeapOffset;
	}

	void RenderNodeDx12::setDsvHeapDescriptorBaseOffset(size_t offset)
	{
		m_dsvDescHeapOffset = offset;
	}
	size_t RenderNodeDx12::getDsvHeapDescriptorBaseOffset() const
	{
		return m_dsvDescHeapOffset;
	}

	void RenderNodeDx12::setRtvHeapSlot(size_t slot)
	{
		m_rtvDescHeapSlot = slot;
	}
	size_t RenderNodeDx12::getRtvHeapSlot() const
	{
		return m_rtvDescHeapSlot;
	}

	void RenderNodeDx12::setDsvHeapSlot(size_t slot)
	{
		m_dsvDescHeapSlot = slot;
	}
	size_t RenderNodeDx12::getDsvHeapSlot() const
	{
		return m_dsvDescHeapSlot;
	}

}