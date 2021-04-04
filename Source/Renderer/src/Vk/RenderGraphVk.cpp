#include <Renderer/Vk/RenderGraphVk.h>

#include <Renderer/Vk/RendererVk.h>
#include <Renderer/Vk/ResourceManagerVk.h>

#include <Renderer/Vk/RenderNodeVk.h>
#include <Renderer/Vk/ComputeNodeVk.h>
#include <Renderer/Vk/RaytraceNodeVk.h>

#include <array>


#if defined(DEBUG) || defined(_DEBUG) 
#define VERBOSE_BARRIER_MERGE_VK
#endif

namespace YAPT
{
	RenderGraphVk::RenderGraphVk(GfxApiHandle h)
		:RenderGraph(h)
	{

	}
	RenderGraphVk::~RenderGraphVk()
	{
		
	}

	void RenderGraphVk::resolveGraphDependenciesInternal()
	{
		generateBarriers();
		mergeReadOnlyBarriers();
		createRenderPasses();
		assert(!"TODO");
	}

	RenderNode* RenderGraphVk::createRenderNodeInternal(const char* name, size_t numberOfConnectionSlots, const RenderGraphNodeSlotDefinition* slotDefinitions)
	{
		return new RenderNodeVk(name, this, numberOfConnectionSlots, slotDefinitions);
	}
	ComputeNode* RenderGraphVk::createComputeNodeInternal(const char* name, size_t numberOfConnectionSlots, const RenderGraphNodeSlotDefinition* slotDefinitions)
	{
		return new ComputeNodeVk(name, this, numberOfConnectionSlots, slotDefinitions);
	}
	RaytraceNode* RenderGraphVk::createRayTraceNodeInternal(const char* name, size_t numberOfConnectionSlots, const RenderGraphNodeSlotDefinition* slotDefinitions)
	{
		return new RaytraceNodeVk(name, this, numberOfConnectionSlots, slotDefinitions);
	}

	void RenderGraphVk::issuePreBarriers(size_t nodeIndex, CommandBufferHandle buffer)
	{
		/*std::array<D3D12_RESOURCE_BARRIER, 256> barriers; //TODO: better solution
		size_t numBarriers = 0;

		BarriersPerNode& barriersPerNode = m_barriers[nodeIndex];

		for (size_t i = 0; i < barriersPerNode.perSlotDesc.size(); ++i)
		{
			ResourceSlotBarrierDescription& barrierSlotDesc = barriersPerNode.perSlotDesc[i];
			if (barrierSlotDesc.isFirstUsageForResource)
			{
				size_t resourceIndex = m_resourceRequirements.getRenderGraphResourceIndex(nodeIndex, i);

				GeneralPerResourceTransitionInformation& info = m_perResourceBarrierInfo[resourceIndex];

				D3D12_RESOURCE_STATES beforeState = info.lastStateInGraph;
				if (info.useOverriddenBeforeState)
				{
					beforeState = info.overriddenBeforeState;
					info.useOverriddenBeforeState = false;
				}

				//TODO: properly check if we need to transition from before state to the actual stage used.
				if (beforeState == barrierSlotDesc.transitionedToState)
				{
					continue;
				}

				for (size_t barrierInd = 0; barrierInd < barrierSlotDesc.currentBeforeBarriers.size(); ++barrierInd)
				{
					barriers[numBarriers] = barrierSlotDesc.currentBeforeBarriers[barrierInd];
					if (barriers[numBarriers].Type == D3D12_RESOURCE_BARRIER_TYPE_TRANSITION)
					{
						barriers[numBarriers].Transition.StateBefore = beforeState;
					}
					assert(barriers[numBarriers].Transition.pResource != nullptr);
					++numBarriers;
				}

			}
			else
			{
				for (size_t barrierInd = 0; barrierInd < barrierSlotDesc.currentBeforeBarriers.size(); ++barrierInd)
				{
					assert(barriers[numBarriers].Transition.pResource != nullptr);
					barriers[numBarriers++] = barrierSlotDesc.currentBeforeBarriers[barrierInd];
					
				}
				
			}
		}

		if (numBarriers > 0)
		{
			buffer->cmdList->ResourceBarrier((UINT)numBarriers, barriers.data());
		}
		*/

	}
	void RenderGraphVk::issuePostBarriers(size_t nodeIndex, CommandBufferHandle buffer)
	{
		//nothing to do before split barriers are done
	}


	void RenderGraphVk::prepareNodeExecution(RenderNodeVk* node, const RenderGraphNodeExecutionContext& context)
	{
		/*
		//depth stencil update
		bool hasDepthStencil = node->hasDepthStencil();
		size_t numberOfColorTargets = node->getNumberOfColorTargets();

		size_t pipelineLength = m_gfxHandle->getResourceManager().getPipelineLength();

		if (hasDepthStencil)
		{
			size_t depthStencilSlot = node->getNodeSlotIndexForDepthStencil();
			RenderGraphResourceId resId = node->getRenderGraphResourceIdForSlot(depthStencilSlot);
			if (isResourceBoundThisFrame(resId))
			{
				//bump the dsv to next "slot" allocated for this node
				node->setDsvHeapSlot((node->getDsvHeapSlot() + 1) % pipelineLength);
				size_t dsvSlot = node->getDsvHeapSlot();

				size_t depthStencilHeapOffset = node->getDsvHeapDescriptorBaseOffset() + dsvSlot;
				const RenderGraphResourceUsage& resourceDesc = m_resourceRequirements.getRenderGraphResourceUsage(node->getSortedIndex(), depthStencilSlot);

				ID3D12Resource* dsvRes =  m_boundRenderGraphResources[resId].textureHandles[0]->resource.get();

				D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc;
				fillDepthStencilViewDesc(resourceDesc.resourceDescription.resourceDimensions, resourceDesc.resourceDescription.resourceFormat, 
					resourceDesc.resourceDescription.accessFlags, (UINT)resourceDesc.mipOffset, (UINT)resourceDesc.arraySliceOffset,
					(UINT)resourceDesc.resourceDescription.arraySliceCount, false,  dsvDesc);

				m_gfxHandle->getResourceManager().getDevice().CreateDepthStencilView(dsvRes, &dsvDesc, m_dsvHeap->getCPUDescriptorHandle(depthStencilHeapOffset));

			}
		}
		//color targets update
		bool needToRecreateRtvs = false;
		for (size_t colorTargetIndex = 0; colorTargetIndex < numberOfColorTargets; ++colorTargetIndex)
		{
			size_t colorTargetSlot = node->getNodeSlotIndexForRenderTargetIndex(colorTargetIndex);
			RenderGraphResourceId resId = node->getRenderGraphResourceIdForSlot(colorTargetSlot);
			if (isResourceBoundThisFrame(resId))
			{
needToRecreateRtvs = true;
break;
			}
		}

		if (needToRecreateRtvs)
		{
			//bump the rtv to next "slot" allocated for this node
			node->setRtvHeapSlot((node->getRtvHeapSlot() + 1) % pipelineLength);
			size_t rtvSlot = node->getRtvHeapSlot();

			for (size_t colorTargetIndex = 0; colorTargetIndex < numberOfColorTargets; ++colorTargetIndex)
			{

				size_t colorTargetSlot = node->getNodeSlotIndexForRenderTargetIndex(colorTargetIndex);
				RenderGraphResourceId resId = node->getRenderGraphResourceIdForSlot(colorTargetSlot);

				size_t rtvHeapOffset = node->getRtvHeapDescriptorBaseOffset() + rtvSlot * numberOfColorTargets;
				const RenderGraphResourceUsage& resourceDesc = m_resourceRequirements.getRenderGraphResourceUsage(node->getSortedIndex(), colorTargetSlot);

				ID3D12Resource* rtvRes = m_boundRenderGraphResources[resId].textureHandles[0]->resource.get();

				D3D12_RENDER_TARGET_VIEW_DESC rtvDesc;
				fillRenderTargetViewDesc(resourceDesc.resourceDescription.resourceDimensions, resourceDesc.resourceDescription.resourceFormat,
					resourceDesc.resourceDescription.accessFlags, (UINT)resourceDesc.mipOffset, (UINT)resourceDesc.arraySliceOffset,
					(UINT)resourceDesc.resourceDescription.arraySliceCount, false, rtvDesc);

				m_gfxHandle->getResourceManager().getDevice().CreateRenderTargetView(rtvRes, &rtvDesc, m_rtvHeap->getCPUDescriptorHandle(rtvHeapOffset + colorTargetIndex));


			}
		}



		//bind
		D3D12_CPU_DESCRIPTOR_HANDLE rtvHeapHandle;
		D3D12_CPU_DESCRIPTOR_HANDLE dsvHeapHandle;
		if (hasDepthStencil)
		{
			dsvHeapHandle = m_dsvHeap->getCPUDescriptorHandle(node->getDsvHeapDescriptorBaseOffset() + node->getDsvHeapSlot());
		}

		if (numberOfColorTargets > 0)
		{
			rtvHeapHandle = m_rtvHeap->getCPUDescriptorHandle(node->getRtvHeapDescriptorBaseOffset() + node->getRtvHeapSlot() * numberOfColorTargets);
		}

		if (numberOfColorTargets > 0 || hasDepthStencil)
		{
			context.cmdBuffer->cmdList->OMSetRenderTargets((UINT)numberOfColorTargets, numberOfColorTargets > 0 ? &rtvHeapHandle : nullptr, TRUE, hasDepthStencil ? &dsvHeapHandle : nullptr);
		}

		*/
	}
	void RenderGraphVk::prepareNodeExecution(ComputeNodeVk* node, const RenderGraphNodeExecutionContext& context)
	{

	}
	void RenderGraphVk::prepareNodeExecution(RaytraceNodeVk* node, const RenderGraphNodeExecutionContext& context)
	{

	}

	void RenderGraphVk::handleClears(RenderGraphNode* node, const RenderGraphNodeExecutionContext& context)
	{
		/*const ClearsPerNode& clears = m_clearsPerNode[node->getSortedIndex()];
		GraphicsCommandListDx12* cmdList = context.cmdBuffer->cmdList;

		ResourceManagerDx12& resMngr = getGfxApiHandle()->getResourceManager();

		for (size_t i = 0; i < clears.slotsToClear.size(); ++i)
		{
			size_t slotIndex = clears.slotsToClear[i];
			const RenderGraphNodeSlotDefinition& def = node->getNodeSlotResourceDefinition(slotIndex);
			RenderGraphResourceId resId = getRenderGraphResourceIdUsedInSlot(node->getSortedIndex(), slotIndex);

			if (def.resourceDescription.resourceUsage == RESOURCE_USAGE_RENDER_TARGET_TEXTURE)
			{
				RenderNodeDx12* rNode = static_cast<RenderNodeDx12*>(node);
				size_t numberOfColorTargets = rNode->getNumberOfColorTargets();
				D3D12_CPU_DESCRIPTOR_HANDLE rtvHeapHandle = m_rtvHeap->getCPUDescriptorHandle(rNode->getRtvHeapDescriptorBaseOffset() + rNode->getRtvHeapSlot() * numberOfColorTargets);
				assert(def.clearValue.type == ClearValue::_ClearValueType::FLOAT);
				vec4p clearValue = def.clearValue.value.fvec;
				FLOAT clearVal[4] = { clearValue.x, clearValue.y, clearValue.z, clearValue.w };

				cmdList->ClearRenderTargetView(rtvHeapHandle, clearVal, 0, nullptr);
			}
			else if ((def.resourceDescription.resourceUsage & RESOURCE_USAGE_DEPTH_STENCIL_TEXTURE) != 0)
			{
				RenderNodeDx12* rNode = static_cast<RenderNodeDx12*>(node);
				D3D12_CPU_DESCRIPTOR_HANDLE dsvHeapHandle = m_dsvHeap->getCPUDescriptorHandle(rNode->getDsvHeapDescriptorBaseOffset() + rNode->getDsvHeapSlot());

				assert(def.clearValue.type == ClearValue::_ClearValueType::DEPTH_STENCIL);
				float d = def.clearValue.value.depthStencil.depth;
				uint8_t s = def.clearValue.value.depthStencil.stencil;
				D3D12_CLEAR_FLAGS flags = (D3D12_CLEAR_FLAGS)0;
				if ((def.resourceDescription.resourceUsage & RESOURCE_USAGE_DEPTH_TEXTURE) != 0)
				{
					flags |= D3D12_CLEAR_FLAG_DEPTH;
				}

				if ((def.resourceDescription.resourceUsage & RESOURCE_USAGE_STENCIL_TEXTURE) != 0)
				{
					flags |= D3D12_CLEAR_FLAG_STENCIL;
				}

				cmdList->ClearDepthStencilView(dsvHeapHandle, flags, d, s, 0, nullptr);

			}

		}*/
	}


	void RenderGraphVk::executeNodesInternal(RenderGraphNode** nodes, size_t nodeCount, const RenderGraphNodeExecutionContext& context)
	{

		for (size_t i = 0; i < nodeCount; ++i)
		{
			size_t nodeIndex = nodes[i]->getSortedIndex();
			issuePreBarriers(nodeIndex, context.cmdBuffer);

			switch (nodes[i]->getType())
			{
				case RenderGraphNode::Type::COMPUTE:
					prepareNodeExecution(static_cast<ComputeNodeVk*>(nodes[i]), context);
					break;
				case RenderGraphNode::Type::RENDER:
					prepareNodeExecution(static_cast<RenderNodeVk*>(nodes[i]), context);
					break;
				case RenderGraphNode::Type::RAYTRACE:
					prepareNodeExecution(static_cast<RaytraceNodeVk*>(nodes[i]), context);
					break;
			}
			handleClears(nodes[i], context);
			invokeNodeCallback(nodes[i],context);
			issuePostBarriers(nodeIndex, context.cmdBuffer);
		}
	}

	void RenderGraphVk::createRenderPasses()
	{
		/*size_t requiredRtvHeapEntries = 0;
		size_t requiredDsvHeapEntries = 0;
		size_t pipelineLength = m_gfxHandle->getResourceManager().getPipelineLength();
		for (size_t nodeIndex = 0; nodeIndex < m_nodes.size(); ++nodeIndex)
		{
			if (m_nodes[nodeIndex]->getType() != RenderGraphNode::Type::RENDER)
			{
				continue;
			}

			RenderNodeDx12* rNode = static_cast<RenderNodeDx12*>(m_nodes[nodeIndex]);
			rNode->setRtvHeapDescriptorBaseOffset(requiredRtvHeapEntries);
			rNode->setDsvHeapDescriptorBaseOffset(requiredDsvHeapEntries);

			size_t attachmentCount = rNode->getNumberOfColorTargets();
			bool hasDepthStencil = rNode->hasDepthStencil();

			requiredRtvHeapEntries += attachmentCount * pipelineLength;
			requiredDsvHeapEntries += (hasDepthStencil ? 1 : 0) * pipelineLength;
		}

		if (requiredRtvHeapEntries > 0)
		{
			m_rtvHeap = m_gfxHandle->getResourceManager().createDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_RTV, requiredRtvHeapEntries);
		}
		if (requiredDsvHeapEntries > 0)
		{
			m_dsvHeap = m_gfxHandle->getResourceManager().createDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_DSV, requiredDsvHeapEntries);
		}		*/
	}

	void RenderGraphVk::calcUsedSubresourceIndices(const RenderGraphResourceDescription& resourceDesc, const RenderGraphResourceUsage& usage, size_t* indicesOut)
	{
		/*uint32_t subResourcesCount = usage.resourceDescription.arraySliceCount + usage.resourceDescription.mipCount;
		for (uint32_t i = 0; i < subResourcesCount; ++i)
		{
			uint32_t arraySlice = usage.arraySliceOffset + i / usage.resourceDescription.mipCount;
			uint32_t mipSlice = usage.mipOffset + i % usage.resourceDescription.mipCount;


			uint32_t subResourceIndex = D3D12CalcSubresource((UINT)mipSlice, (UINT)arraySlice, 0u, (UINT)resourceDesc.mipCount, (UINT)resourceDesc.arraySliceCount);
			indicesOut[i] = (size_t)subResourceIndex;
		}*/
	}

	void RenderGraphVk::generateBarriers()
	{
		/*auto areSubresourcesSharingPreviousState = [](const std::vector<D3D12_RESOURCE_STATES>& states, size_t* indices, size_t numberOfIndices) -> bool
		{
			D3D12_RESOURCE_STATES state;
			bool stateIsShared = true;
			for (size_t i = 0; i < numberOfIndices; ++i)
			{
				size_t subResourceIndex = indices[i];

				if (i == 0)
				{
					state = states[subResourceIndex];
				}
				else
				{
					if (state != states[subResourceIndex])
					{
						stateIsShared = false;
						break;
					}
				}
			}
			return stateIsShared;
		};

		auto needsUavBarrier = [](const std::vector<D3D12_RESOURCE_STATES>& previousStates, size_t* indices, size_t numberOfIndices) -> bool
		{
			bool needsUavBarrier = false;
			for (size_t i = 0; i < numberOfIndices; ++i)
			{
				D3D12_RESOURCE_STATES state = previousStates[indices[i]];
				if (state & D3D12_RESOURCE_STATE_UNORDERED_ACCESS)
				{
					needsUavBarrier = true;
					break;
				}
			}

			return needsUavBarrier;
		};


		std::vector<D3D12_RESOURCE_STATES> subResourceStates;
		subResourceStates.reserve(512);
		std::vector<size_t> subResourceIndices;
		subResourceIndices.reserve(512);

		const size_t nodeCount = getNodeCount();
		m_barriers.resize(nodeCount);
		for (size_t nodeIndex = 0; nodeIndex < nodeCount; ++nodeIndex)
		{
			BarriersPerNode& perNodeBarriers = m_barriers[nodeIndex];
			perNodeBarriers.perSlotDesc.resize(getNodes()[nodeIndex]->getNumberOfSlots());
		}

		m_perResourceBarrierInfo.resize(m_resourceRequirements.getNumberOfRenderGraphResourceDescriptions());


		for (size_t resourceIndex = 0; resourceIndex < m_resourceRequirements.getNumberOfRenderGraphResourceDescriptions(); ++resourceIndex)
		{
			const RenderGraphResourceDescription& resourceDesc = m_resourceRequirements.getRenderGraphResourceDescription(resourceIndex);

			subResourceStates.resize(size_t(resourceDesc.mipCount * resourceDesc.arraySliceCount));


			const NodeSlotIdentifier* nodeSlotIdentifiers;
			size_t numberOfNodeSlotIdentifiers;
			m_resourceRequirements.getNodeSlotsUsingResource(resourceIndex, nodeSlotIdentifiers, numberOfNodeSlotIdentifiers);

			for (size_t resourceUsageIndex = 0; resourceUsageIndex < numberOfNodeSlotIdentifiers; ++resourceUsageIndex)
			{
				size_t nodeIndex = nodeSlotIdentifiers[resourceUsageIndex].sortedNodeIndex;
				size_t slotIndex = nodeSlotIdentifiers[resourceUsageIndex].slotIndex;

				BarriersPerNode& perNodeBarriers = m_barriers[nodeIndex];
				ResourceSlotBarrierDescription& perSlotBarriers = perNodeBarriers.perSlotDesc[slotIndex];
				const RenderGraphResourceUsage& usageInThisSlot = m_resourceRequirements.getRenderGraphResourceUsage(nodeIndex, slotIndex);
				uint32_t numberOfSubresources = usageInThisSlot.resourceDescription.arraySliceCount * usageInThisSlot.resourceDescription.mipCount;
				subResourceIndices.resize(numberOfSubresources);
				calcUsedSubresourceIndices(resourceDesc, usageInThisSlot, subResourceIndices.data());

				D3D12_RESOURCE_STATES statesInNode = getD3D12StateFromResourceUsage(usageInThisSlot);
				bool fullResourceUsed = isUsingFullResource(resourceDesc, usageInThisSlot);
				
				bool isFirstUsage = resourceUsageIndex == 0;
				bool subResourcesShareState = areSubresourcesSharingPreviousState(subResourceStates, subResourceIndices.data(), subResourceIndices.size());
				bool issueUavBarrier = needsUavBarrier(subResourceStates, subResourceIndices.data(), subResourceIndices.size());
				bool transitionFullResource = fullResourceUsed && subResourcesShareState;

				size_t numberOfTransitionBarriers = transitionFullResource ? 1 : numberOfSubresources;


				perSlotBarriers.beforeBarriersPerResource.resize(numberOfTransitionBarriers + (issueUavBarrier ? 1 : 0));
				perSlotBarriers.transitionedToState = statesInNode;

				//transition barriers
				for (size_t i = 0; i < numberOfTransitionBarriers; ++i)
				{
					D3D12_RESOURCE_BARRIER& barrier = perSlotBarriers.beforeBarriersPerResource[i];
					barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
					barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
					barrier.Transition.StateAfter = statesInNode;

					size_t subResourceIndex = subResourceIndices[i];

					if (transitionFullResource)
					{
						barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
					}
					else
					{
						barrier.Transition.Subresource = (UINT)subResourceIndices[i];
					}

					if (!isFirstUsage)
					{
						barrier.Transition.StateBefore = subResourceStates[subResourceIndex];
					}
				}

				//uav barriers
				if (issueUavBarrier)
				{
					D3D12_RESOURCE_BARRIER& barrier = perSlotBarriers.beforeBarriersPerResource.back();
					barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
					barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
					
				}
				
				perSlotBarriers.hasUavBarrier = issueUavBarrier;
				perSlotBarriers.numberOfTransitionBarriers = numberOfTransitionBarriers;

				if (isFirstUsage)
				{
					perSlotBarriers.isFirstUsageForResource = true;
				}

				//replicate current state to subresources
				for (size_t i = 0; i < numberOfSubresources; ++i)
				{
					size_t subResourceIndex = subResourceIndices[i];
					subResourceStates[subResourceIndex] = statesInNode;
				}

				
			}
			m_perResourceBarrierInfo[resourceIndex].lastStateInGraph = getD3D12StateFromResourceUsage(getLastStateForResource(resourceIndex));
		}*/
	}


	void RenderGraphVk::mergeReadOnlyBarriers()
	{
		/*auto canMergeReadonlyBarriers = [](const ResourceSlotBarrierDescription& a, const ResourceSlotBarrierDescription& b)
		{
			bool canMerge = true;

			bool isReadOnlyA = (D3D12_ALL_WRITE_STATES & a.transitionedToState) == 0;
			bool isReadOnlyB = (D3D12_ALL_WRITE_STATES & b.transitionedToState) == 0;

			canMerge = canMerge && isReadOnlyA && isReadOnlyB;

			//for now, only merge if subresources match. In reality, could just merge them too
			canMerge = canMerge && (a.numberOfTransitionBarriers == b.numberOfTransitionBarriers);
			if (canMerge)
			{
				for (size_t i = 0; i < a.numberOfTransitionBarriers; ++i)
				{
					if (a.beforeBarriersPerResource[i].Transition.Subresource != b.beforeBarriersPerResource[i].Transition.Subresource)
					{
						canMerge = false;
						break;
					}
				}
			}

			return canMerge;
		};

		struct BarriersMerged
		{
			size_t fromNode;
			size_t fromSlot;

			size_t toNode;
			size_t toSlot;
		};

		std::vector<BarriersMerged> mergedBarriers;
		mergedBarriers.reserve(128);

		for (size_t resourceIndex = 0; resourceIndex < m_resourceRequirements.getNumberOfRenderGraphResourceDescriptions(); ++resourceIndex)
		{

			mergedBarriers.clear();
			const RenderGraphResourceDescription& resourceDesc = m_resourceRequirements.getRenderGraphResourceDescription(resourceIndex);

			const NodeSlotIdentifier* nodeSlotIdentifiers;
			size_t numberOfNodeSlotIdentifiers;
			m_resourceRequirements.getNodeSlotsUsingResource(resourceIndex, nodeSlotIdentifiers, numberOfNodeSlotIdentifiers);

			for (size_t resourceUsageIndex = numberOfNodeSlotIdentifiers - 1; resourceUsageIndex > 0; --resourceUsageIndex)
			{
				size_t nodeIndex = nodeSlotIdentifiers[resourceUsageIndex].sortedNodeIndex;
				size_t slotIndex = nodeSlotIdentifiers[resourceUsageIndex].slotIndex;

				size_t beforeNodeIndex = nodeSlotIdentifiers[resourceUsageIndex - 1].sortedNodeIndex;
				size_t beforeSlotIndex = nodeSlotIdentifiers[resourceUsageIndex - 1].slotIndex;

				ResourceSlotBarrierDescription& barriersThis = m_barriers[nodeIndex].perSlotDesc[slotIndex];
				ResourceSlotBarrierDescription& barriersBefore = m_barriers[beforeNodeIndex].perSlotDesc[beforeSlotIndex];


				if (canMergeReadonlyBarriers(barriersBefore, barriersThis))
				{
#ifdef VERBOSE_BARRIER_MERGE_DX12
					YAPT_LOG_DEBUG("Merging barriers from (node index %d, slot index %d) to (node index %d, slot index %d). Original state from %d, original state to %d, new state %d",
						nodeIndex, slotIndex, beforeNodeIndex, beforeSlotIndex, barriersThis.transitionedToState, barriersBefore.transitionedToState, barriersThis.transitionedToState | barriersBefore.transitionedToState);
#endif

					D3D12_RESOURCE_STATES newState = barriersBefore.transitionedToState | barriersThis.transitionedToState;
					//assumes that subresources are identical
					for (size_t i = 0; i < barriersBefore.numberOfTransitionBarriers; ++i)
					{
						barriersBefore.beforeBarriersPerResource[i].Transition.StateAfter = newState;
					}
					//clear from
					barriersThis.beforeBarriersPerResource.clear();
					barriersThis.transitionedToState = newState;

					mergedBarriers.push_back({ nodeIndex, slotIndex, beforeNodeIndex, beforeSlotIndex });
					
				}
			}

			//check if we merged the last usage barriers so we give back correct "state" the resource will be at the end of the graph
			if (mergedBarriers.size() > 0 && mergedBarriers[0].fromNode == nodeSlotIdentifiers[numberOfNodeSlotIdentifiers - 1].sortedNodeIndex)
			{
				ResourceStateDescription resourceStateDesc = m_lastStateInGraph[resourceIndex];
				size_t lastMergedNodeIndex = nodeSlotIdentifiers[numberOfNodeSlotIdentifiers - 1].sortedNodeIndex;
				for (size_t i = 0; i < mergedBarriers.size(); ++i)
				{
					if (lastMergedNodeIndex == mergedBarriers[i].fromNode)
					{
						const RenderGraphResourceUsage& usage = m_resourceRequirements.getRenderGraphResourceUsage(mergedBarriers[i].toNode, mergedBarriers[i].toSlot);

						resourceStateDesc.accessFlags |= usage.resourceDescription.accessFlags;
						resourceStateDesc.resourceUsage |= usage.resourceDescription.resourceUsage;
						resourceStateDesc.shaderStagesUsedIn |= usage.resourceDescription.shaderStages;

						lastMergedNodeIndex = mergedBarriers[i].toNode;
					}
				}
				m_lastStateInGraph[resourceIndex] = resourceStateDesc;
			}

		}

		for (size_t resourceIndex = 0; resourceIndex < m_resourceRequirements.getNumberOfRenderGraphResourceDescriptions(); ++resourceIndex)
		{
			const NodeSlotIdentifier* nodeSlotIdentifiers;
			size_t numberOfNodeSlotIdentifiers;
			m_resourceRequirements.getNodeSlotsUsingResource(resourceIndex, nodeSlotIdentifiers, numberOfNodeSlotIdentifiers);

			const NodeSlotIdentifier& nodeSlot = nodeSlotIdentifiers[numberOfNodeSlotIdentifiers - 1];
			const ResourceSlotBarrierDescription& barrierDesc = m_barriers[nodeSlot.sortedNodeIndex].perSlotDesc[nodeSlot.slotIndex];
			m_perResourceBarrierInfo[resourceIndex].lastStateInGraph = barrierDesc.transitionedToState;
		}*/

	}


	bool RenderGraphVk::isUsingFullResource(const RenderGraphResourceDescription& resourceDesc,  const RenderGraphResourceUsage& to)
	{
		 
		bool isFullResource = to.mipOffset == 0 
		&& to.arraySliceOffset == 0
		&& resourceDesc.arraySliceCount == to.resourceDescription.arraySliceCount
		&& resourceDesc.mipCount == to.resourceDescription.mipCount;
		

		return isFullResource;
	}



	void RenderGraphVk::resourcesBoundToPipeline(RenderGraphResourceId id, const ResourceStateDescription previousState, size_t numberOfResourcesBound)
	{
		updateBarriersForResource(id, previousState, numberOfResourcesBound);
	}

	
	void RenderGraphVk::updateBarriersForResource(RenderGraphResourceId id, const ResourceStateDescription previousState, size_t numberOfResourcesBound)
	{
		/*auto injectResourceToBarrier = [](ID3D12Resource* resource, D3D12_RESOURCE_BARRIER& barrier)
		{
			switch (barrier.Type)
			{
			case D3D12_RESOURCE_BARRIER_TYPE_TRANSITION:
				barrier.Transition.pResource = resource;
				break;
			case D3D12_RESOURCE_BARRIER_TYPE_UAV:
				barrier.UAV.pResource = resource;
				break;
			case D3D12_RESOURCE_BARRIER_TYPE_ALIASING:
				assert(!"not supported/handled");
			}
		};

		D3D12_RESOURCE_STATES beforeState = getD3D12StateFromResourceUsage(previousState);
		//inject resource to relevant barriers
		const NodeSlotIdentifier* nodeSlotIdentifiers;
		size_t numberOfNodeSlotIdentifiers;
		m_resourceRequirements.getNodeSlotsUsingResource(id, nodeSlotIdentifiers, numberOfNodeSlotIdentifiers);

		for (size_t i = 0; i < numberOfNodeSlotIdentifiers; ++i)
		{
			size_t nodeIndex = nodeSlotIdentifiers[i].sortedNodeIndex;
			size_t slotIndex = nodeSlotIdentifiers[i].slotIndex;
			ResourceSlotBarrierDescription& barrierDescs = m_barriers[nodeIndex].perSlotDesc[slotIndex];

			//copy per resource barriers to current barriers (the amount of barriers could potentially change because of more/less resources bound to slot)
			size_t barriersPerResourceCount = barrierDescs.beforeBarriersPerResource.size();
			barrierDescs.currentBeforeBarriers.resize(barriersPerResourceCount * numberOfResourcesBound);



			for (size_t resIndex = 0; resIndex < numberOfResourcesBound; ++resIndex)
			{

				ID3D12Resource* resource = nullptr;
				if (m_boundRenderGraphResources[id].type == BoundResourceType::TEXTURE)
				{
					resource = m_boundRenderGraphResources[id].textureHandles[resIndex]->resource;
				}
				else if (m_boundRenderGraphResources[id].type == BoundResourceType::BUFFER)
				{
					resource = m_boundRenderGraphResources[id].bufferHandles[resIndex]->resource;
				}
				else
				{
					assert(!"unknown binding");
				}


				assert(resource != nullptr);

				for (size_t barrierIndex = 0; barrierIndex < barriersPerResourceCount; ++barrierIndex)
				{
					size_t dstBarrierIndex = resIndex * barriersPerResourceCount + barrierIndex;
					barrierDescs.currentBeforeBarriers[dstBarrierIndex] = barrierDescs.beforeBarriersPerResource[barrierIndex];
					injectResourceToBarrier(resource, barrierDescs.currentBeforeBarriers[dstBarrierIndex]);
				}
			}



			if (barrierDescs.isFirstUsageForResource)
			{
				m_perResourceBarrierInfo[id].useOverriddenBeforeState = true;
				m_perResourceBarrierInfo[id].overriddenBeforeState = beforeState;
			}

		}*/
	}
	
	
}