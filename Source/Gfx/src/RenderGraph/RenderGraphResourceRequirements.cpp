#include <Gfx/RenderGraph/RenderGraphResourceRequirements.h>
#include <Gfx/GfxBasicTypesUtility.h>

#include <algorithm>

namespace YAPT
{
	RenderGraphResourceRequirements::RenderGraphResourceRequirements()
	{

	}
	RenderGraphResourceRequirements::~RenderGraphResourceRequirements()
	{

	}

	const RenderGraphResourceDescription& RenderGraphResourceRequirements::getRenderGraphResourceDescription(size_t index) const 
	{ 
		assert(index < m_requiredRenderGraphResourceDescriptions.size());
		return m_requiredRenderGraphResourceDescriptions[index]; 
	}

	void RenderGraphResourceRequirements::resolve(RenderGraphNode** sortedNodes, size_t nodeCount)
	{
		resolveResourcesRequired(sortedNodes, nodeCount);

	}


	void RenderGraphResourceRequirements::reset()
	{
		m_requiredRenderGraphResourceDescriptions.clear();
		m_resourceIndexToNodeSlots.clear();
		m_perNodeResourceUsages.clear();
		m_nodeSlotToResourceIndex.clear();
	}


	RenderGraphResourceUsage& RenderGraphResourceRequirements::getRenderGraphResourceUsageIn(size_t nodeIndex, size_t slot)
	{
		return m_perNodeResourceUsages[nodeIndex].renderGraphResourceUsage[slot];
	}

	size_t RenderGraphResourceRequirements::getRenderGraphResourceIndex(size_t nodeSortedIndex, size_t slot) const
	{
		return m_nodeSlotToResourceIndex[nodeSortedIndex].resourceIndexPerSlot[slot];
	}

	void RenderGraphResourceRequirements::getNodeSlotsUsingResource(size_t resourceIndex,const NodeSlotIdentifier*& nodeSlotIdsOut, size_t& numberOfNodeSlotIds)
	{
		nodeSlotIdsOut = m_resourceIndexToNodeSlots[resourceIndex].usedInNodeSlotIdentifiers.data();
		numberOfNodeSlotIds = m_resourceIndexToNodeSlots[resourceIndex].usedInNodeSlotIdentifiers.size();
	}

	bool RenderGraphResourceRequirements::appendNodeSlotDefinitionToResourceDescription(const RenderGraphNodeSlotDefinition& nodeslotDefinition, RenderGraphResourceDescription& targetResource)
	{

		bool compatible = true;

		targetResource.accessFlags |= nodeslotDefinition.resourceDescription.accessFlags;
		targetResource.resourceUsage |= nodeslotDefinition.resourceDescription.resourceUsage;
		targetResource.arraySliceCount = std::max(targetResource.arraySliceCount, nodeslotDefinition.resourceDescription.arraySliceCount);
		targetResource.mipCount = std::max(targetResource.mipCount, nodeslotDefinition.resourceDescription.mipCount);

		//resolve what usages dimensions are potentially compatible (for now only array types with similar dimensions otherwise
		if (targetResource.resourceDimensions != nodeslotDefinition.resourceDescription.resourceDimensions)
		{
			if (getArrayType(targetResource.resourceDimensions) == nodeslotDefinition.resourceDescription.resourceDimensions)
			{
				targetResource.resourceDimensions = nodeslotDefinition.resourceDescription.resourceDimensions;
			}
			else if (getArrayType(nodeslotDefinition.resourceDescription.resourceDimensions) == targetResource.resourceDimensions)
			{

			}
			else
			{
				compatible = false;
			}



		}

		compatible = compatible && (targetResource.resourceFormat == nodeslotDefinition.resourceDescription.resourceFormat || targetResource.resourceFormat == ResourceFormat::UNKNOWN || nodeslotDefinition.resourceDescription.resourceFormat == ResourceFormat::UNKNOWN);

		return compatible;
	}

	void RenderGraphResourceRequirements::replicateRenderGraphResourceIndexToAllConnectedSlots(RenderGraphNode* node, size_t slot, size_t index)
	{
		size_t resourceIndex = getRenderGraphResourceIndex(node->getSortedIndex(), slot);
		if (resourceIndex != size_t(-1))
		{
			assert(resourceIndex == index);
			return;
		}

		m_nodeSlotToResourceIndex[node->getSortedIndex()].resourceIndexPerSlot[slot] = index;

		for (size_t inputIndex = 0; inputIndex < node->getNumberOfInputEdges(slot); ++inputIndex)
		{
			const RenderGraphNodeEdge* edge = node->getInputEdge(slot, inputIndex);
			replicateRenderGraphResourceIndexToAllConnectedSlots(edge->fromNode, edge->fromSlot, index);
		}

		for (size_t outputIndex = 0; outputIndex < node->getNumberOfOutputEdges(slot); ++outputIndex)
		{
			const RenderGraphNodeEdge* edge = node->getOutputEdge(slot, outputIndex);
			replicateRenderGraphResourceIndexToAllConnectedSlots(edge->toNode, edge->toSlot, index);
		}
	}

	void RenderGraphResourceRequirements::generateRenderGraphResourceDefinition(size_t resourceIndex, RenderGraphNode** sortedNodes, size_t nodeCount)
	{
		const NodeSlotIdentifier* nodeSlotIdentifiers;
		size_t numberOfNodeSlotIdentifiers;
		getNodeSlotsUsingResource(resourceIndex, nodeSlotIdentifiers, numberOfNodeSlotIdentifiers);
		
		RenderGraphResourceDescription& desc = m_requiredRenderGraphResourceDescriptions[resourceIndex];
		
		//initialize resource usage with the first usage and then append/merge subsequent usages to generate full resource description
		
		{
			size_t nodeIndex = nodeSlotIdentifiers[0].sortedNodeIndex;
			size_t slotIndex = nodeSlotIdentifiers[0].slotIndex;

			const RenderGraphNodeSlotDefinition& nodeSlotDef = sortedNodes[nodeIndex]->getNodeSlotResourceDefinition(slotIndex);

			desc = nodeSlotDef.resourceDescription;
		}

		bool hasUnknownResourceFormats = desc.resourceFormat == ResourceFormat::UNKNOWN;

		for (size_t usageIndex = 1; usageIndex < numberOfNodeSlotIdentifiers; ++usageIndex)
		{
			size_t nodeIndex = nodeSlotIdentifiers[usageIndex].sortedNodeIndex;
			size_t slotIndex = nodeSlotIdentifiers[usageIndex].slotIndex;

			const RenderGraphNodeSlotDefinition& cdef =  sortedNodes[nodeIndex]->getNodeSlotResourceDefinition(slotIndex);
			//if the resource had unknown format (ie. "I don't care"), it will be assumed the same as the first usage

			if (desc.resourceFormat == ResourceFormat::UNKNOWN && cdef.resourceDescription.resourceFormat != ResourceFormat::UNKNOWN)
			{
				desc.resourceFormat = cdef.resourceDescription.resourceFormat;
			}

			if (cdef.resourceDescription.resourceFormat == ResourceFormat::UNKNOWN)
			{
				hasUnknownResourceFormats = true;
			}

			bool compatible = appendNodeSlotDefinitionToResourceDescription(cdef, desc);
			assert(compatible);
		}

		if (hasUnknownResourceFormats)
		{
			for (size_t usageIndex = 0; usageIndex < numberOfNodeSlotIdentifiers; ++usageIndex)
			{
				size_t nodeIndex = nodeSlotIdentifiers[usageIndex].sortedNodeIndex;
				size_t slotIndex = nodeSlotIdentifiers[usageIndex].slotIndex;

				sortedNodes[nodeIndex]->setResolvedResourceFormat(slotIndex, desc.resourceFormat);
			}
		}

	}
	void RenderGraphResourceRequirements::generateRenderGraphResourceUsages(size_t resourceIndex, RenderGraphNode** sortedNodes, size_t nodeCount)
	{
		const NodeSlotIdentifier* nodeSlotIdentifiers;
		size_t numberOfNodeSlotIdentifiers;
		getNodeSlotsUsingResource(resourceIndex, nodeSlotIdentifiers, numberOfNodeSlotIdentifiers);

		RenderGraphResourceDescription& desc = m_requiredRenderGraphResourceDescriptions[resourceIndex];

		size_t fullResourceUsedNodeSlotIndex = size_t(-1);

		for (size_t i = numberOfNodeSlotIdentifiers - 1; i != size_t(-1); --i)
		{
			size_t nodeIndex = nodeSlotIdentifiers[i].sortedNodeIndex;
			size_t slotIndex = nodeSlotIdentifiers[i].slotIndex;
			
			RenderGraphNode* node = sortedNodes[nodeIndex];
			const RenderGraphNodeSlotDefinition& nodeSlotDef = node->getNodeSlotResourceDefinition(slotIndex);

			if (nodeSlotDef.resourceDescription.arraySliceCount == desc.arraySliceCount && nodeSlotDef.resourceDescription.mipCount == desc.mipCount)
			{
				fullResourceUsedNodeSlotIndex = (size_t)i;
				break;
			}

		}
		assert(fullResourceUsedNodeSlotIndex != size_t(-1));

		size_t numberOfUsagesDefined = 0;

		size_t nodeIndex = nodeSlotIdentifiers[fullResourceUsedNodeSlotIndex].sortedNodeIndex;
		size_t slotIndex = nodeSlotIdentifiers[fullResourceUsedNodeSlotIndex].slotIndex;

		RenderGraphNode* node = sortedNodes[nodeIndex];

		propagateRenderGraphResourceUsage(node, slotIndex, 0, 0, numberOfUsagesDefined);

		assert(numberOfUsagesDefined == numberOfNodeSlotIdentifiers);

	}

	void RenderGraphResourceRequirements::propagateRenderGraphResourceUsage(RenderGraphNode* node, size_t slotIndex, uint32_t sliceOffset, uint32_t mipOffset, size_t& numberOfUsagesDefined)
	{
		const RenderGraphNodeSlotDefinition& nodeSlotDef = node->getNodeSlotResourceDefinition(slotIndex);
		RenderGraphResourceUsage& usage = getRenderGraphResourceUsageIn(node->getSortedIndex(), slotIndex);

		//check if already visited
		if (usage.arraySliceOffset != uint32_t(-1)) return;

		usage.resourceDescription = nodeSlotDef.resourceDescription;
		usage.arraySliceOffset = sliceOffset;
		usage.mipOffset = mipOffset;

		++numberOfUsagesDefined;

		size_t numberOfInputEdges = node->getNumberOfInputEdges(slotIndex);
		for (size_t i = 0; i < numberOfInputEdges; ++i)
		{
			const RenderGraphNodeEdge* edge = node->getInputEdge(slotIndex, i);
			propagateRenderGraphResourceUsage(edge->fromNode, edge->fromSlot, sliceOffset + edge->toArrayOffset, mipOffset + edge->toMipOffset, numberOfUsagesDefined);
		}

		size_t numberOfOutputEdges = node->getNumberOfOutputEdges(slotIndex);
		for (size_t i = 0; i < numberOfOutputEdges; ++i)
		{
			const RenderGraphNodeEdge* edge = node->getOutputEdge(slotIndex, i);
			propagateRenderGraphResourceUsage(edge->toNode, edge->toSlot, sliceOffset, mipOffset, numberOfUsagesDefined);
		}
	}

	void RenderGraphResourceRequirements::resolveResourcesRequired(RenderGraphNode** sortedNodes, size_t nodeCount)
	{
		//initialize structures
		m_perNodeResourceUsages.resize(nodeCount);
		m_nodeSlotToResourceIndex.resize(nodeCount);

		for (size_t nodeIndex = 0; nodeIndex < nodeCount; ++nodeIndex)
		{
			RenderGraphNode* node = sortedNodes[nodeIndex];
			size_t slotCount = node->getNumberOfSlots();

			RenderGraphResourceUsage defaultUsage;
			defaultUsage.arraySliceOffset = uint32_t(-1);
			defaultUsage.mipOffset = uint32_t(-1);
			m_perNodeResourceUsages[nodeIndex].renderGraphResourceUsage.resize(slotCount, defaultUsage);
			m_nodeSlotToResourceIndex[nodeIndex].resourceIndexPerSlot.resize(slotCount, size_t(-1)); //initialize with unspecified resourceindex

		}

		//traverse graph and resolve required resource Ids and nodes using them 
		for (size_t nodeIndex = 0; nodeIndex < nodeCount; ++nodeIndex)
		{
			RenderGraphNode* node = sortedNodes[nodeIndex];
			size_t slotCount = node->getNumberOfSlots();
			for (size_t slotIndex = 0; slotIndex < slotCount; ++slotIndex)
			{
				size_t index = getRenderGraphResourceIndex(node->getSortedIndex(), slotIndex);

				//if the slot does not have valid index, create a new index and replicate it to all slots connected to this
				if (index == size_t(-1))
				{
					index = m_requiredRenderGraphResourceDescriptions.size();
					m_requiredRenderGraphResourceDescriptions.resize(index + 1);
					m_resourceIndexToNodeSlots.resize(m_requiredRenderGraphResourceDescriptions.size());
					replicateRenderGraphResourceIndexToAllConnectedSlots(node, slotIndex, index);
				}

				//add this nodeslot as the user of the resource in 'index'
				m_resourceIndexToNodeSlots[index].usedInNodeSlotIdentifiers.emplace_back(node->getSortedIndex(), slotIndex);
			}
		}

		

		//finally resolve the actual resources needed and resouce usages per nodeslot
		for (size_t resourceIndex = 0; resourceIndex < m_requiredRenderGraphResourceDescriptions.size(); ++resourceIndex)
		{
			generateRenderGraphResourceDefinition(resourceIndex, sortedNodes, nodeCount);
			generateRenderGraphResourceUsages(resourceIndex, sortedNodes, nodeCount);
		}


	}




}