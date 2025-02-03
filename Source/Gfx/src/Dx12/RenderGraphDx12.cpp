#include <Gfx/Dx12/RenderGraphDx12.h>
		  
#include <Gfx/Dx12/RendererDx12.h>
#include <Gfx/Dx12/ResourceManagerDx12.h>
		  
#include <Gfx/Dx12/RenderNodeDx12.h>
#include <Gfx/Dx12/ComputeNodeDx12.h>
#include <Gfx/Dx12/RaytraceNodeDx12.h>
#include <Gfx/Dx12/DescriptorHeapDx12.h>
#include <Gfx/Dx12/YaptToDx12Conversions.h>
#include <Gfx/Dx12/Dx12MiscUtils.h>
#include <Gfx/Dx12/d3dx12.h>

#include <array>


#if defined(DEBUG) || defined(_DEBUG) 
#define VERBOSE_BARRIER_MERGE_DX12
#endif

namespace YAPT
{
	class SwapChainNodeDx12 : public SwapChainNode
	{
	public:

		SwapChainNodeDx12(const char* name, RenderGraph* graph, RenderGraphNodeSlotDefinition* definitions, size_t definitionCount)
			:SwapChainNode(name, graph, definitions, definitionCount)
		{

		}

		virtual ~SwapChainNodeDx12()
		{
		}


	};


	RenderGraphDx12::RenderGraphDx12(GfxApiHandle h)
		:RenderGraph(h),
		m_rtvHeap(nullptr),
		m_dsvHeap(nullptr)
	{

	}
	RenderGraphDx12::~RenderGraphDx12()
	{
		

		if (m_rtvHeap)
		{
			m_rtvHeap->destroy();
		}
		if (m_dsvHeap)
		{
			m_dsvHeap->destroy();
		}
	}

	void RenderGraphDx12::resolveGraphDependenciesInternal()
	{
		generateBarriers();
		mergeReadOnlyBarriers();
		if (useSplitBarriers)
		{
			generateSplitBarriers();
		}
		createRenderTargetResources();
	}

	RenderNode* RenderGraphDx12::createRenderNodeInternal(const char* name, size_t numberOfConnectionSlots, const RenderGraphNodeSlotDefinition* slotDefinitions)
	{
		return new RenderNodeDx12(name, this, numberOfConnectionSlots, slotDefinitions);
	}
	ComputeNode* RenderGraphDx12::createComputeNodeInternal(const char* name, size_t numberOfConnectionSlots, const RenderGraphNodeSlotDefinition* slotDefinitions)
	{
		return new ComputeNodeDx12(name, this, numberOfConnectionSlots, slotDefinitions);
	}
	RaytraceNode* RenderGraphDx12::createRayTraceNodeInternal(const char* name, size_t numberOfConnectionSlots, const RenderGraphNodeSlotDefinition* slotDefinitions)
	{
		return new RaytraceNodeDx12(name, this, numberOfConnectionSlots, slotDefinitions);
	}

	SwapChainNode* RenderGraphDx12::createSwapChainNodeInternal(const char* name)
	{
		RenderGraphNodeSlotDefinition slotDef = { ResourceDimension::TEXTURE_2D,
			ResourceFormat::RGBA8_SRGB,
			RESOURCE_USAGE_PRESENTABLE_TEXTURE,
			ACCESS_FLAGS_READ,
			SHADERSTAGE_NONE,
			1,
			1
		};

		return new SwapChainNodeDx12(name, this, &slotDef, 1);
	}

	void RenderGraphDx12::issuePreBarriers(size_t nodeIndex, CommandBufferHandle cmdList)
	{
		std::array<D3D12_RESOURCE_BARRIER, 256> barriers; //TODO: better solution
		size_t numBarriers = 0;

		BarriersPerNode& barriersPerNode = m_barriers[nodeIndex];

		for (size_t i = 0; i < barriersPerNode.perSlotDesc.size(); ++i)
		{
			ResourceSlotBarrierDescription& barrierSlotDesc = barriersPerNode.perSlotDesc[i];
			size_t resourceIndex = m_resourceRequirements.getRenderGraphResourceIndex(nodeIndex, i);
			GeneralPerResourceTransitionInformation& info = m_perResourceBarrierInfo[resourceIndex];

			//use either overridden barriers (this one time) or the default before barriers
			if (barrierSlotDesc.isFirstUsageForResource && info.useOverriddenBeforeState)
			{
				
				for (size_t barrierInd = 0; barrierInd < barrierSlotDesc.overriddenBeforeBarriers.size(); ++barrierInd)
				{
					barriers[numBarriers++] = barrierSlotDesc.overriddenBeforeBarriers[barrierInd];

				}

				info.useOverriddenBeforeState = false;

			}
			else
			{
				for (size_t barrierInd = 0; barrierInd < barrierSlotDesc.currentBeforeBarriers.size(); ++barrierInd)
				{
					//check if we are trying to transition to same state (can happen for example if this is the first usage in graph and the "wrap around" barrier has the same state.
					if (barrierSlotDesc.currentBeforeBarriers[barrierInd].Type == D3D12_RESOURCE_BARRIER_TYPE_TRANSITION && barrierSlotDesc.currentBeforeBarriers[barrierInd].Transition.StateAfter == barrierSlotDesc.currentBeforeBarriers[barrierInd].Transition.StateBefore)
						continue;

					barriers[numBarriers++] = barrierSlotDesc.currentBeforeBarriers[barrierInd];
					
				}
				
			}
		}

		if (numBarriers > 0)
		{
			cmdList->cmdList->ResourceBarrier((UINT)numBarriers, barriers.data());
		}
		

	}
	void RenderGraphDx12::issuePostBarriers(size_t nodeIndex, CommandBufferHandle buffer)
	{
		//nothing to do before split barriers are done
	}


	void RenderGraphDx12::prepareNodeExecution(RenderNodeDx12* node, const RenderGraphNodeExecutionContext& context)
	{
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


	}
	void RenderGraphDx12::prepareNodeExecution(ComputeNodeDx12* node, const RenderGraphNodeExecutionContext& context)
	{

	}
	void RenderGraphDx12::prepareNodeExecution(RaytraceNodeDx12* node, const RenderGraphNodeExecutionContext& context)
	{

	}

	void RenderGraphDx12::handleClears(RenderGraphNode* node, const RenderGraphNodeExecutionContext& context)
	{
		const ClearsPerNode& clears = m_clearsPerNode[node->getSortedIndex()];
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
				vec4p clearValue = glm::make_vec4(def.clearValue.value.fvec);
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

		}
	}


	void RenderGraphDx12::executeNodesInternal(RenderGraphNode** nodes, size_t nodeCount, const RenderGraphNodeExecutionContext& context)
	{

		for (size_t i = 0; i < nodeCount; ++i)
		{
			size_t nodeIndex = nodes[i]->getSortedIndex();
			issuePreBarriers(nodeIndex, context.cmdBuffer);

			switch (nodes[i]->getType())
			{
				case RenderGraphNode::Type::COMPUTE:
					prepareNodeExecution(static_cast<ComputeNodeDx12*>(nodes[i]), context);
					break;
				case RenderGraphNode::Type::RENDER:
					prepareNodeExecution(static_cast<RenderNodeDx12*>(nodes[i]), context);
					handleClears(nodes[i], context);
					break;
				case RenderGraphNode::Type::RAYTRACE:
					prepareNodeExecution(static_cast<RaytraceNodeDx12*>(nodes[i]), context);
					break;
			}
			
			invokeNodeCallback(nodes[i],context);
			issuePostBarriers(nodeIndex, context.cmdBuffer);
		}
	}

	void RenderGraphDx12::createRenderTargetResources()
	{
		size_t requiredRtvHeapEntries = 0;
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
		}		
	}

	void RenderGraphDx12::calcUsedSubresourceIndices(const RenderGraphResourceDescription& resourceDesc, const RenderGraphResourceUsage& usage, size_t* indicesOut)
	{
		uint32_t subResourcesCount = usage.resourceDescription.arraySliceCount + usage.resourceDescription.mipCount;
		for (uint32_t i = 0; i < subResourcesCount; ++i)
		{
			uint32_t arraySlice = usage.arraySliceOffset + i / usage.resourceDescription.mipCount;
			uint32_t mipSlice = usage.mipOffset + i % usage.resourceDescription.mipCount;


			uint32_t subResourceIndex = D3D12CalcSubresource((UINT)mipSlice, (UINT)arraySlice, 0u, (UINT)resourceDesc.mipCount, (UINT)resourceDesc.arraySliceCount);
			indicesOut[i] = (size_t)subResourceIndex;
		}
	}


	void RenderGraphDx12::generateBarriers()
	{
		auto areSubresourcesSharingPreviousState = [](const std::vector<D3D12_RESOURCE_STATES>& states, size_t* indices, size_t numberOfIndices) -> bool
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
		std::vector<size_t> subResourceIndicesToTransition;
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

				//check which subresources need to actually transition
				subResourceIndicesToTransition.clear();
				if (transitionFullResource)
				{
					if (statesInNode != subResourceStates[0])
					{
						subResourceIndicesToTransition.push_back(0);
					}
				}
				else
				{
					for (size_t i = 0; i < numberOfSubresources; ++i)
					{
						if (statesInNode != subResourceStates[i])
						{
							subResourceIndicesToTransition.push_back(i);
						}
					}
				}

				size_t numberOfTransitionBarriers = subResourceIndicesToTransition.size();


				perSlotBarriers.preGeneratedBeforeBarriersPerResource.resize(numberOfTransitionBarriers + (issueUavBarrier ? 1 : 0));
				perSlotBarriers.transitionedToState = statesInNode;

				//transition barriers
				for (int i = 0; i < numberOfTransitionBarriers; ++i)
				{
					D3D12_RESOURCE_BARRIER& barrier = perSlotBarriers.preGeneratedBeforeBarriersPerResource[i];
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
					D3D12_RESOURCE_BARRIER& barrier = perSlotBarriers.preGeneratedBeforeBarriersPerResource.back();
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

				m_perResourceBarrierInfo[resourceIndex].lastStateInGraph = getD3D12StateFromResourceUsage(getLastStateForResource(resourceIndex)); //setup the d3d12 last state in graph (note that this might still change in merging barriers step)
			}
		}
	}

	D3D12_RESOURCE_STATES RenderGraphDx12::getLastStateInGraphInternal(size_t resourceId)
	{
		return m_perResourceBarrierInfo[resourceId].lastStateInGraph;
	}

	void RenderGraphDx12::mergeReadOnlyBarriers()
	{
		auto canMergeReadonlyBarriers = [](const ResourceSlotBarrierDescription& a, const ResourceSlotBarrierDescription& b)
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
					if (a.preGeneratedBeforeBarriersPerResource[i].Transition.Subresource != b.preGeneratedBeforeBarriersPerResource[i].Transition.Subresource)
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
						barriersBefore.preGeneratedBeforeBarriersPerResource[i].Transition.StateAfter = newState;
					}
					//clear from
					barriersThis.preGeneratedBeforeBarriersPerResource.clear();
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

			m_perResourceBarrierInfo[resourceIndex].lastStateInGraph = getD3D12StateFromResourceUsage(getLastStateForResource(resourceIndex));

			//go through resource usages and set the correct "before" barrier state for barriers transitioning from the last rendergraph state
			for (size_t nodeSlotId = 0; nodeSlotId < numberOfNodeSlotIdentifiers; ++nodeSlotId)
			{
				const NodeSlotIdentifier& nodeSlot = nodeSlotIdentifiers[nodeSlotId];
				ResourceSlotBarrierDescription& barrierDesc = m_barriers[nodeSlot.sortedNodeIndex].perSlotDesc[nodeSlot.slotIndex];
				if (barrierDesc.isFirstUsageForResource)
				{
					for (size_t barrierInd = 0; barrierInd < barrierDesc.numberOfTransitionBarriers; ++barrierInd)
					{
						barrierDesc.preGeneratedBeforeBarriersPerResource[barrierInd].Transition.StateBefore = m_perResourceBarrierInfo[resourceIndex].lastStateInGraph;
					}
				}

			}

			
			
		}

	}
	void RenderGraphDx12::generateSplitBarriers()
	{

	}


	D3D12_RESOURCE_STATES RenderGraphDx12::getD3D12StateFromResourceUsage(const RenderGraphResourceUsage& usage)
	{
		return yaptUsageToDx12ResourceStates(usage.resourceDescription.resourceUsage, usage.resourceDescription.accessFlags, usage.resourceDescription.shaderStages);
	}

	D3D12_RESOURCE_STATES RenderGraphDx12::getD3D12StateFromResourceUsage(const ResourceStateDescription& previousState)
	{
		return yaptUsageToDx12ResourceStates(previousState.resourceUsage, previousState.accessFlags, previousState.shaderStagesUsedIn);
	}

	bool RenderGraphDx12::isUsingFullResource(const RenderGraphResourceDescription& resourceDesc,  const RenderGraphResourceUsage& to)
	{
		 
		bool isFullResource = to.mipOffset == 0 
		&& to.arraySliceOffset == 0
		&& resourceDesc.arraySliceCount == to.resourceDescription.arraySliceCount
		&& resourceDesc.mipCount == to.resourceDescription.mipCount;
		

		return isFullResource;
	}



	void RenderGraphDx12::resourcesBoundToPipeline(RenderGraphResourceId id, size_t numberOfResourcesBound)
	{
		updateBarriersForResource(id, numberOfResourcesBound);
	}

	void RenderGraphDx12::updateBarriersForResource(RenderGraphResourceId id, size_t numberOfResourcesBound)
	{
		auto injectResourceToBarrier = [](ID3D12Resource* resource, D3D12_RESOURCE_BARRIER& barrier)
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
			size_t barriersPerResourceCount = barrierDescs.preGeneratedBeforeBarriersPerResource.size();
			barrierDescs.currentBeforeBarriers.resize(barriersPerResourceCount * numberOfResourcesBound);
			
			//when we bind the resource for the first time to the graph, might need to transition from arbitrary state to whatever its needed to be in. After first iteration in the graph, the state is known (laststate of the graph) so no need to do this after first use
			if (barrierDescs.isFirstUsageForResource)
			{
				m_perResourceBarrierInfo[id].useOverriddenBeforeState = true;
				barrierDescs.overriddenBeforeBarriers.clear();
			}

			for (size_t resIndex = 0; resIndex < numberOfResourcesBound; ++resIndex)
			{

				ID3D12Resource* resource = nullptr;
				ResourceStateTrackerDx12 *stateTracker;
				if (m_boundRenderGraphResources[id].type == BoundResourceType::TEXTURE)
				{
					TextureHandle handle = m_boundRenderGraphResources[id].textureHandles[resIndex];
					resource = handle->resource;
					stateTracker = &handle->lastSeenState; //assumes that the state for the resource is not different between subresources!! this might not be the case in reality
				}
				else if (m_boundRenderGraphResources[id].type == BoundResourceType::BUFFER)
				{
					BufferHandle handle = m_boundRenderGraphResources[id].bufferHandles[resIndex];
					resource = handle->resource;
					stateTracker = &handle->lastSeenState;
				}
				else
				{
					assert(!"unknown binding");
				}


				assert(resource != nullptr);

				//copy pregenerated barriers
				for (size_t barrierIndex = 0; barrierIndex < barriersPerResourceCount; ++barrierIndex)
				{
					size_t dstBarrierIndex = resIndex * barriersPerResourceCount + barrierIndex;
					barrierDescs.currentBeforeBarriers[dstBarrierIndex] = barrierDescs.preGeneratedBeforeBarriersPerResource[barrierIndex];
					injectResourceToBarrier(resource, barrierDescs.currentBeforeBarriers[dstBarrierIndex]);
				}

				//if this is the first time usage of the resource in the graph, need to potentially transition from external state, so collect the "before" states for the transition barriers above 
				if (barrierDescs.isFirstUsageForResource)
				{
					for (size_t barrierIndex = 0; barrierIndex < barriersPerResourceCount; ++barrierIndex)
					{
						if (barrierDescs.preGeneratedBeforeBarriersPerResource[barrierIndex].Type == D3D12_RESOURCE_BARRIER_TYPE_TRANSITION)
						{
							size_t subResourceIndex = barrierDescs.preGeneratedBeforeBarriersPerResource[barrierIndex].Transition.Subresource;
							subResourceIndex = subResourceIndex == D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES ? 0 : subResourceIndex;
							D3D12_RESOURCE_STATES state = stateTracker->getStateForSubResource(subResourceIndex);

							if (state != barrierDescs.preGeneratedBeforeBarriersPerResource[barrierIndex].Transition.StateAfter)
							{
								D3D12_RESOURCE_BARRIER overrideBarrier = barrierDescs.preGeneratedBeforeBarriersPerResource[barrierIndex];
								injectResourceToBarrier(resource, overrideBarrier);
								overrideBarrier.Transition.StateBefore = state;

								barrierDescs.overriddenBeforeBarriers.push_back(overrideBarrier);
							}

						}

					}
					
				}

			}
		}

		//mark the last seen state to be the last state in the graph
		
		for (size_t resIndex = 0; resIndex < numberOfResourcesBound; ++resIndex)
		{

			ResourceStateTrackerDx12* stateTracker;
			bool stateDecaysToCommon = false;
			if (m_boundRenderGraphResources[id].type == BoundResourceType::TEXTURE)
			{
				TextureHandle handle = m_boundRenderGraphResources[id].textureHandles[resIndex];
				stateTracker = &handle->lastSeenState; //assumes that the state for the resource is not different between subresources!! this might not be the case in reality
			}
			else if (m_boundRenderGraphResources[id].type == BoundResourceType::BUFFER)
			{
				BufferHandle handle = m_boundRenderGraphResources[id].bufferHandles[resIndex];
				stateTracker = &handle->lastSeenState;
				stateDecaysToCommon = true;
			}
			else
			{
				assert(!"unknown bound resource type");
			}

			D3D12_RESOURCE_STATES lastStateForResource = stateDecaysToCommon ? D3D12_RESOURCE_STATE_COMMON : getLastStateInGraphInternal(id);
			stateTracker->setSharedState(lastStateForResource);
		}


	}

	
}