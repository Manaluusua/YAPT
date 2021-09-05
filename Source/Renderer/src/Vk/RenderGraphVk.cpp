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
		generateBarriersAndRenderPasses();
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

	VkRenderPass RenderGraphVk::createRenderPass(RenderNode* node)
	{

	}


	void RenderGraphVk::calcUsedSubresourceIndices(const RenderGraphResourceDescription& resourceDesc, const RenderGraphResourceUsage& usage, size_t* indicesOut)
	{
		uint32_t subResourcesCount = usage.resourceDescription.arraySliceCount + usage.resourceDescription.mipCount;
		for (uint32_t i = 0; i < subResourcesCount; ++i)
		{
			uint32_t arraySlice = usage.arraySliceOffset + i / usage.resourceDescription.mipCount;
			uint32_t mipSlice = usage.mipOffset + i % usage.resourceDescription.mipCount;


			uint32_t subResourceIndex = mipSlice + arraySlice * usage.resourceDescription.mipCount;
			indicesOut[i] = (size_t)subResourceIndex;
		}
	}

	void RenderGraphVk::generateBarriersAndRenderPasses()
	{
		
		
		const size_t nodeCount = getNodeCount();
		m_barriers.resize(nodeCount);
		for (size_t nodeIndex = 0; nodeIndex != nodeCount; ++nodeIndex)
		{
			if (m_nodes[nodeIndex]->getType() == RenderGraphNode::Type::RENDER)
			{
				RenderNodeVk* rNode = static_cast<RenderNodeVk*>(m_nodes[nodeIndex]);
				VkRenderPass rp = createRenderPass(rNode);
				size_t rpIndex = m_renderPasses.size();
				m_renderPasses.push_back(rp);
				rNode->setRenderPassIndex(rpIndex);
			}

		}

		std::vector<AccessFlagsAndLayout> subResourceStates;
		subResourceStates.reserve(512);
		std::vector<size_t> subResourceIndices;
		subResourceIndices.reserve(512);
		std::vector<size_t> subResourceIndicesToTransition;
		subResourceIndicesToTransition.reserve(512);

		const size_t nodeCount = getNodeCount();
		m_barriers.resize(nodeCount);
		for (size_t nodeIndex = 0; nodeIndex != nodeCount; ++nodeIndex)
		{
			BarriersPerNode& perNodeBarriers = m_barriers[nodeIndex];
			perNodeBarriers.perSlotDesc.resize(getNodes()[nodeIndex]->getNumberOfSlots());
		}

		m_perResourceBarrierInfo.resize(m_resourceRequirements.getNumberOfRenderGraphResourceDescriptions());


		for (size_t nodeIndex = 0; nodeIndex != nodeCount; ++nodeIndex)
		{
			BarriersPerNode& perNodeBarriers = m_barriers[nodeIndex];
			RenderGraphNode* node = getNodes()[nodeIndex];
			size_t numberOfSlots = node->getNumberOfSlots();

			uint32_t toQueueFamilyIndex = m_queueFamilyIndexPerNode[node->getSortedIndex()];

			for (size_t slotIndex = 0; slotIndex != numberOfSlots; ++slotIndex)
			{
				ResourceSlotBarrierDescription& perSlotBarriers = perNodeBarriers.perSlotDesc[slotIndex];
				const RenderGraphResourceUsage& usageInThisSlot = m_resourceRequirements.getRenderGraphResourceUsage(nodeIndex, slotIndex);

				perSlotBarriers.srcStages = 0;

				size_t numberOfInputEdges = node->getNumberOfInputEdges(slotIndex);
				RenderGraphResourceId resID = getRenderGraphResourceIdUsedInSlot(nodeIndex, slotIndex);

				const NodeSlotIdentifier* nodeSlotIds;
				size_t numberOfNodeSlotIds;

				m_resourceRequirements.getNodeSlotsUsingResource(resID, nodeSlotIds, numberOfNodeSlotIds);

				bool isFirstUsage = numberOfInputEdges == 0;
				perSlotBarriers.isFirstUsageForResource = isFirstUsage;

				VkImageMemoryBarrier imgBarrier;
				VkBufferMemoryBarrier bufferBarrier;
				VkMemoryBarrier memoryBarrier;

				if (isFirstUsage)
				{
					size_t lastUsedNode = nodeSlotIds[numberOfNodeSlotIds - 1].sortedNodeIndex;
					size_t lastUsedSlot = nodeSlotIds[numberOfNodeSlotIds - 1].slotIndex;

					const RenderGraphResourceUsage& lastUsage = m_resourceRequirements.getRenderGraphResourceUsage(lastUsedNode, lastUsedSlot);
					uint32_t fromQueueFamilyIndex = m_queueFamilyIndexPerNode[lastUsedNode];
					if (GeneratedBarrierTypeMask generatedBarriersMask = createBarrierIfRequired(usageInThisSlot.resourceDescription.resourceDimensions,
						lastUsage.resourceDescription.resourceUsage, lastUsage.resourceDescription.accessFlags, usageInThisSlot.resourceDescription.resourceUsage, usageInThisSlot.resourceDescription.accessFlags,
						lastUsage.arraySliceOffset, lastUsage.mipOffset, lastUsage.resourceDescription.arraySliceCount, lastUsage.resourceDescription.mipCount, fromQueueFamilyIndex, toQueueFamilyIndex,
						imgBarrier, bufferBarrier, memoryBarrier))
					{

					}

				}
				else
				{

					for (size_t inputIndex = 0; inputIndex != numberOfInputEdges; ++inputIndex)
					{
						const RenderGraphNodeEdge* edge = node->getInputEdge(slotIndex, inputIndex);
						const RenderGraphResourceUsage& previousUsage = m_resourceRequirements.getRenderGraphResourceUsage(edge->fromNode->getSortedIndex(), edge->fromSlot);

						uint32_t fromQueueFamilyIndex = m_queueFamilyIndexPerNode[edge->fromNode->getSortedIndex()];

						if (GeneratedBarrierTypeMask generatedBarriersMask = createBarrierIfRequired(usageInThisSlot.resourceDescription.resourceDimensions,
							previousUsage.resourceDescription.resourceUsage, previousUsage.resourceDescription.accessFlags, usageInThisSlot.resourceDescription.resourceUsage, usageInThisSlot.resourceDescription.accessFlags,
							previousUsage.arraySliceOffset, previousUsage.mipOffset, previousUsage.resourceDescription.arraySliceCount, previousUsage.resourceDescription.mipCount, fromQueueFamilyIndex, toQueueFamilyIndex,
							imgBarrier, bufferBarrier, memoryBarrier))
						{
							if (usageInThisSlot.resourceDescription.resourceUsage & (RESOURCE_USAGE_RENDER_TARGET_TEXTURE | RESOURCE_USAGE_DEPTH_TEXTURE | RESOURCE_USAGE_STENCIL_TEXTURE) != 0) //if used in renderpass, have renderpass define the dependencies. first usage is an exception since we don't want to create another renderpass if the previous state passed in is different than when RP was created
							{
								continue;
							}

						}

					}
				}
			}
		}

		/*

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
				RenderGraphNode* node = getNodes()[nodeIndex];

				BarriersPerNode& perNodeBarriers = m_barriers[nodeIndex];
				ResourceSlotBarrierDescription& perSlotBarriers = perNodeBarriers.perSlotDesc[slotIndex];
				const RenderGraphResourceUsage& usageInThisSlot = m_resourceRequirements.getRenderGraphResourceUsage(nodeIndex, slotIndex);
				uint32_t numberOfSubresources = usageInThisSlot.resourceDescription.arraySliceCount * usageInThisSlot.resourceDescription.mipCount;
				subResourceIndices.resize(numberOfSubresources);
				calcUsedSubresourceIndices(resourceDesc, usageInThisSlot, subResourceIndices.data());

				AccessFlagsAndLayout accessFlagsAndLayout = accessFlagsAndLayoutFromUsage(usageInThisSlot);
				bool fullResourceUsed = isUsingFullResource(resourceDesc, usageInThisSlot);
				
				bool isFirstUsage = resourceUsageIndex == 0;
				bool subResourcesShareState = areSubresourcesSharingPreviousUsageAndAccess(subResourceStates, subResourceIndices.data(), subResourceIndices.size());
				bool issueUavBarrier = needsUavBarrier(, slotIndex);
				bool transitionFullResource = fullResourceUsed && subResourcesShareState;

				//check which subresources need to actually transition
				subResourceIndicesToTransition.clear();
				if (transitionFullResource)
				{
					if (accessFlagsAndLayout != subResourceStates[0])
					{
						subResourceIndicesToTransition.push_back(0);
					}
				}
				else
				{
					for (size_t i = 0; i < numberOfSubresources; ++i)
					{
						if (accessFlagsAndLayout == subResourceStates[i])
						{
							subResourceIndicesToTransition.push_back(i);
						}
					}
				}

				size_t numberOfTransitionBarriers = subResourceIndicesToTransition.size();


				perSlotBarriers.beforeBarriersPerResource.resize(numberOfTransitionBarriers + (issueUavBarrier ? 1 : 0));
				perSlotBarriers.transitionedToState = accessFlagsAndLayout;

				//transition barriers
				for (int i = 0; i < numberOfTransitionBarriers; ++i)
				{
					D3D12_RESOURCE_BARRIER& barrier = perSlotBarriers.beforeBarriersPerResource[i];
					barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
					barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
					barrier.Transition.StateAfter = statesInNode;

					size_t subResourceIndex = subResourceIndicesToTransition[i];

					if (transitionFullResource)
					{
						barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
					}
					else
					{
						barrier.Transition.Subresource = (UINT)subResourceIndex;
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
			*/
		
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

	bool RenderGraphVk::getVkAccessMaskTransition(ResourceUsage usageFrom, AccessFlags accessFlagsFrom, ResourceUsage usageTo, AccessFlags accessFlagsTo, VkAccessFlags& from, VkAccessFlags& to)
	{
		if (usageFrom == usageTo && accessFlagsFrom == accessFlagsTo) return false;
		assert(!"TODO");

	}
	bool RenderGraphVk::getVkImageLayoutTransition(ResourceUsage usageFrom, AccessFlags accessFlagsFrom, ResourceUsage usageTo, AccessFlags accessFlagsTo, VkImageLayout& from, VkImageLayout& to)
	{
		assert(!"TODO");
	}

	RenderGraphVk::GeneratedBarrierTypeMask RenderGraphVk::createBarrierIfRequired(ResourceDimension resDimension, ResourceUsage usageFrom, AccessFlags accessFlagsFrom, ResourceUsage usageTo, AccessFlags accessFlagsTo,
		uint32_t arrayOffset, uint32_t mipOffset, uint32_t arrayCount, uint32_t mipCount, uint32_t srcQueueFamilyIndex, uint32_t dstQueueFamilyIndex,
		VkImageMemoryBarrier& imageBarrierOut, VkBufferMemoryBarrier& bufferBarrierOut, VkMemoryBarrier& memoryBarrier)
	{
		VkAccessFlags fromAccess;
		VkAccessFlags toAccess;
		RenderGraphVk::GeneratedBarrierTypeMask mask = GENERATED_BARRIER_TYPE_NONE;


		if (!getVkAccessMaskTransition(usageFrom, accessFlagsFrom, usageTo, accessFlagsTo, fromAccess, toAccess))
			return mask;

		switch (resDimension)
		{
		case ResourceDimension::BUFFER:
		{
			mask |= GENERATED_BARRIER_TYPE_BUFFER;
		}
			
		break;
		case ResourceDimension::TEXTURE_1D:
		case ResourceDimension::TEXTURE_1D_ARRAY:
		case ResourceDimension::TEXTURE_2D:
		case ResourceDimension::TEXTURE_2D_ARRAY:
		case ResourceDimension::TEXTURE_3D:
		case ResourceDimension::TEXTURE_CUBEMAP:
		case ResourceDimension::TEXTURE_CUBEMAP_ARRAY:
		{
			VkImageLayout layoutFrom;
			VkImageLayout layoutTo;
			getVkImageLayoutTransition(usageFrom, accessFlagsFrom, usageTo, accessFlagsTo, layoutFrom, layoutTo);

			imageBarrierOut.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			imageBarrierOut.pNext = NULL;
			imageBarrierOut.srcQueueFamilyIndex = srcQueueFamilyIndex;
			imageBarrierOut.dstQueueFamilyIndex = dstQueueFamilyIndex;

			imageBarrierOut.oldLayout = layoutFrom;
			imageBarrierOut.newLayout = layoutTo;
			imageBarrierOut.srcAccessMask = fromAccess;
			imageBarrierOut.dstAccessMask = toAccess;

			mask |= GENERATED_BARRIER_TYPE_IMAGE;
		}
			
		break;

		default:
			assert(!"not implemented/unknown");
		}
	}
	
	
}