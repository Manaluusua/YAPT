#ifndef YAPT_SHARED_RENDERGRAPHRESOURCEREQUIREMENTS_H
#define YAPT_SHARED_RENDERGRAPHRESOURCEREQUIREMENTS_H
#include <Gfx/GfxApi.h>
#include "RenderGraphNode.h"
#include <vector>

namespace YAPT
{

	struct RenderGraphResourceUsage
	{
		RenderGraphResourceDescription resourceDescription;
		uint32_t arraySliceOffset;
		uint32_t mipOffset;
	};

	struct NodeSlotIdentifier
	{
		NodeSlotIdentifier(size_t node, size_t slot)
			:sortedNodeIndex(node),
			slotIndex(slot)
		{}
		size_t sortedNodeIndex;
		size_t slotIndex;
	};

	class RenderGraphResourceRequirements
	{
	public:
		RenderGraphResourceRequirements();
		~RenderGraphResourceRequirements();

		size_t getRenderGraphResourceIndex(size_t nodeSortedIndex, size_t slot) const;
		void getNodeSlotsUsingResource(size_t resourceIndex, const NodeSlotIdentifier*& nodeSlotIdsOut, size_t& numberOfNodeSlotIds);

		const RenderGraphResourceDescription& getRenderGraphResourceDescription(size_t index) const; 

		size_t getNumberOfRenderGraphResourceDescriptions() const { return m_requiredRenderGraphResourceDescriptions.size(); }

		const RenderGraphResourceUsage& getRenderGraphResourceUsage(size_t nodeIndex, size_t slot) { return getRenderGraphResourceUsageIn(nodeIndex, slot); }

		void resolve(RenderGraphNode** sortedNodes, size_t nodeCount);
		void reset();
	private:

		struct RenderGraphResourceUsagePerSlot
		{
			std::vector<RenderGraphResourceUsage> renderGraphResourceUsage;
		};

		struct NodeSlotToResourceIndexMapping
		{
			std::vector<size_t> resourceIndexPerSlot;
		};

		struct ResourceIndextoNodeSlotMapping
		{
			std::vector<NodeSlotIdentifier> usedInNodeSlotIdentifiers;
		};

		
		RenderGraphResourceUsage& getRenderGraphResourceUsageIn(size_t nodeIndex, size_t slot);

		void resolveResourcesRequired(RenderGraphNode** sortedNodes, size_t nodeCount);

		void replicateRenderGraphResourceIndexToAllConnectedSlots(RenderGraphNode* node, size_t slot, size_t index);

		void generateRenderGraphResourceDefinition(size_t resourceIndex, RenderGraphNode** sortedNodes, size_t nodeCount);
		
		void generateRenderGraphResourceUsages(size_t resourceIndex, RenderGraphNode** sortedNodes, size_t nodeCount);
		void propagateRenderGraphResourceUsage(RenderGraphNode* node, size_t slotIndex, uint32_t sliceOffset, uint32_t mipOffset, size_t &numberOfUsagesDefined);

		bool appendNodeSlotDefinitionToResourceDescription(const RenderGraphNodeSlotDefinition& nodeSlotDefinition, RenderGraphResourceDescription& targetResource);

		std::vector<RenderGraphResourceDescription> m_requiredRenderGraphResourceDescriptions;
		std::vector<ResourceIndextoNodeSlotMapping> m_resourceIndexToNodeSlots;

		std::vector<RenderGraphResourceUsagePerSlot> m_perNodeResourceUsages;
		std::vector<NodeSlotToResourceIndexMapping> m_nodeSlotToResourceIndex;
	};
	


}

#endif