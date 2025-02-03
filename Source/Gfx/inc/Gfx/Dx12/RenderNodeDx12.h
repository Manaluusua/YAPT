#pragma once

#include <Gfx/RenderGraph/RenderNode.h>

namespace YAPT
{
	class RenderGraphDx12;
	class RenderNodeDx12 final : public RenderNode
	{
		friend class RenderGraphDx12;
	public:
		virtual RenderPassHandle getRenderPassHandle() final;

		void setRtvHeapDescriptorBaseOffset(size_t offset);
		size_t getRtvHeapDescriptorBaseOffset() const;

		void setDsvHeapDescriptorBaseOffset(size_t offset);
		size_t getDsvHeapDescriptorBaseOffset() const;

		void setRtvHeapSlot(size_t slot);
		size_t getRtvHeapSlot() const;

		void setDsvHeapSlot(size_t slot);
		size_t getDsvHeapSlot() const;
		
	protected:
		virtual void renderGraphCompilationComplete() override;

	private:
		RenderNodeDx12(const char* name, RenderGraph* graph, size_t numberOfConnectionSlots, const RenderGraphNodeSlotDefinition* slotDefinitions);
		virtual ~RenderNodeDx12();

		RenderPassDeclarationDx12 m_renderPassDecl;

		size_t m_rtvDescHeapOffset;
		size_t m_rtvDescHeapSlot;
		size_t m_dsvDescHeapOffset;
		size_t m_dsvDescHeapSlot;

	};
}
