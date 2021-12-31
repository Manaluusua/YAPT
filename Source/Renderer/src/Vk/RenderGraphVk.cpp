#include <Renderer/Vk/RenderGraphVk.h>

#include <Renderer/Vk/RendererVk.h>
#include <Renderer/Vk/ResourceManagerVk.h>

#include <Renderer/Vk/RenderNodeVk.h>
#include <Renderer/Vk/ComputeNodeVk.h>
#include <Renderer/Vk/RaytraceNodeVk.h>

#include <array>

#include <Renderer/Vk/YaptToVkConversions.h>


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
		uint32_t queueFamily = getGfxApiHandle()->getGraphicsQueue().queueFamilyIndex;

		m_queueFamilyIndexPerNode.resize(getNodeCount(), queueFamily);

		generateBarriersAndRenderPasses();

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

	void RenderGraphVk::fillAttachmentDescription(RenderNode* node, size_t slot, VkImageLayout initialLayout, VkImageLayout finalLayout, VkAttachmentDescription& descOut)
	{
		size_t nodeIndex = node->getSortedIndex();
		size_t numberOfInputEdges = node->getNumberOfInputEdges(slot);
		size_t numberOfOutputEdges = node->getNumberOfOutputEdges(slot);
		
		const RenderGraphNodeSlotDefinition& slotDef = node->getNodeSlotResourceDefinition(slot);
		RenderGraphResourceId resID = getRenderGraphResourceIdUsedInSlot(nodeIndex, slot);

		const RenderGraphResourceDescription& resDesc = getRenderGraphResourceDescription(resID);
		bool isDepthStencil = node->getNodeSlotIndexForDepthStencil() == slot;
		bool isFirstUsage = numberOfInputEdges == 0;
		bool isLastUsage = numberOfOutputEdges == 0;

		bool forceLoad = (slotDef.flags & RGNS_FLAG_ALWAYS_REQUIRE_LOAD) != 0;
		bool forceStore = (slotDef.flags & RGNS_FLAG_ALWAYS_REQUIRE_STORE) != 0;
		bool hasClear = slotDef.clearFrequency == RenderNodeClearFrequency::ALWAYS;

		descOut.flags = 0;
		descOut.format = yaptFormatToVk(resDesc.resourceFormat);
		descOut.samples = VK_SAMPLE_COUNT_1_BIT; //TODO: support MS

		VkAttachmentLoadOp loadOp = (forceLoad || !isFirstUsage) ? VK_ATTACHMENT_LOAD_OP_LOAD : VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		loadOp = hasClear ? VK_ATTACHMENT_LOAD_OP_CLEAR : loadOp;

		VkAttachmentStoreOp storeOp = (forceStore || !isLastUsage) ? VK_ATTACHMENT_STORE_OP_STORE : VK_ATTACHMENT_STORE_OP_DONT_CARE;

		if (isDepthStencil)
		{
			descOut.stencilLoadOp = loadOp;
			descOut.stencilStoreOp = storeOp;
			descOut.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			descOut.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		}
		else
		{
			descOut.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			descOut.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			descOut.loadOp = loadOp;
			descOut.storeOp = storeOp;
		}
		
		descOut.initialLayout = initialLayout;
		descOut.finalLayout = finalLayout;
	}

	VkRenderPass RenderGraphVk::createRenderPass(RenderNode* node)
	{
		VkRenderPass renderPass;
		VkRenderPassCreateInfo createInfo{};
		VkSubpassDescription subPassDesc;
		std::vector<VkAttachmentDescription> attachments;
		std::vector<VkAttachmentReference> attachmentReferences;

		VkSubpassDependency subPassDependencies[2]; //We don't in reality always need 2 dependencies since if it's for example the first usage for all resources in the pass, we have barrier taking care of dependencies and no need to do it here
		
		
		size_t numberOfColorAttachments = node->getNumberOfColorTargets();
		bool hasDepthStencil = node->hasDepthStencil();
		size_t depthStencilNodeSlot = hasDepthStencil ? node->getNodeSlotIndexForDepthStencil() : -1;
		attachments.resize(numberOfColorAttachments + (hasDepthStencil ? 1 : 0));
		attachmentReferences.resize(attachments.size());
		

		createInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		createInfo.pNext = VK_NULL_HANDLE;
		createInfo.flags = 0;
		createInfo.subpassCount = 1;
		createInfo.pSubpasses = &subPassDesc;
		createInfo.attachmentCount = (uint32_t)attachments.size();
		createInfo.pAttachments = attachments.data();
		createInfo.dependencyCount = (uint32_t)countOf(subPassDependencies);
		createInfo.pDependencies = subPassDependencies;

		size_t nodeIndex = node->getSortedIndex();
		uint32_t toQueueFamilyIndex = m_queueFamilyIndexPerNode[node->getSortedIndex()];

		VkAccessFlags accessBeforeSubpass = 0;
		VkAccessFlags accessAfterSubpass = 0;
		VkAccessFlags accessDuringSubpass = 0;

		VkPipelineStageFlags stagesBeforeSubpass = 0;
		VkPipelineStageFlags stagesDuringSubpass = 0;
		VkPipelineStageFlags stagesAfterSubpass = 0;

		for (size_t attInd = 0; attInd < attachments.size(); ++attInd)
		{
			bool isDepthStencilAttachment = attInd >= numberOfColorAttachments;
			size_t slotIndex = isDepthStencilAttachment ? node->getNodeSlotIndexForDepthStencil() : node->getNodeSlotIndexForRenderTargetIndex(attInd);
			VkAttachmentDescription& attachDesc = attachments[attInd];
			VkAttachmentReference& attachRef = attachmentReferences[attInd];

			size_t numberOfInputEdges = node->getNumberOfInputEdges(slotIndex);
			size_t numberOfOutputEdges = node->getNumberOfOutputEdges(slotIndex);
			bool isFirstUsage = numberOfInputEdges == 0;
			bool isLastUsage = numberOfOutputEdges == 0;

			//for now only one input edge per attachment 
			assert(numberOfInputEdges < 2);

			VkAccessFlags accessCurrent;
			VkImageLayout currentLayout;

			VkAccessFlags accessBefore;
			VkImageLayout initialLayout;

			VkAccessFlags accessAfter;
			VkImageLayout finalLayout;
			{
				const RenderGraphNodeSlotDefinition& slotDef = node->getNodeSlotResourceDefinition(slotIndex);
				accessCurrent = yaptUsageAccessToVkAccess(slotDef.resourceDescription.resourceUsage, slotDef.resourceDescription.accessFlags);
				currentLayout = yaptUsageToVkImageLayout(slotDef.resourceDescription.resourceUsage, slotDef.resourceDescription.accessFlags);
				stagesDuringSubpass |= yaptShaderStagesToVk(slotDef.resourceDescription.shaderStages);
				accessDuringSubpass |= accessCurrent;
			}
			

			if (isFirstUsage)
			{
				accessBefore = accessCurrent;
				initialLayout = currentLayout;
				//stagesBeforeSubpass |= VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT; //we use barrier to handle this so no need for this

			}
			else
			{
				const RenderGraphNodeEdge* edge = node->getInputEdge(slotIndex, 0);
				RenderGraphNode* prevNode = edge->fromNode;
				size_t prevSlot = edge->fromSlot;

				const RenderGraphNodeSlotDefinition& slotDef = prevNode->getNodeSlotResourceDefinition(prevSlot);
				initialLayout = yaptUsageToVkImageLayout(slotDef.resourceDescription.resourceUsage, slotDef.resourceDescription.accessFlags);
				accessBefore = yaptUsageAccessToVkAccess(slotDef.resourceDescription.resourceUsage, slotDef.resourceDescription.accessFlags);
				stagesBeforeSubpass |= yaptShaderStagesToVk(slotDef.resourceDescription.shaderStages);
				accessBeforeSubpass |= accessBefore;
			}

			if (isLastUsage)
			{
				finalLayout = currentLayout;
				accessAfter = accessCurrent;
				//stagesAfterSubpass |= VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT; //we use barrier to handle this so no need for this
			}
			else
			{
				{
					const RenderGraphNodeEdge* edge = node->getOutputEdge(slotIndex, 0);
					RenderGraphNode* nextNode = edge->toNode;
					size_t nextSlot = edge->toSlot;

					const RenderGraphNodeSlotDefinition& slotDef = nextNode->getNodeSlotResourceDefinition(nextSlot);
					finalLayout = yaptUsageToVkImageLayout(slotDef.resourceDescription.resourceUsage, slotDef.resourceDescription.accessFlags);
					accessAfter = yaptUsageAccessToVkAccess(slotDef.resourceDescription.resourceUsage, slotDef.resourceDescription.accessFlags);

					stagesAfterSubpass |= yaptShaderStagesToVk(slotDef.resourceDescription.shaderStages);
					accessAfterSubpass |= accessAfter;
				}

				for (size_t outputIndex = 1; outputIndex < numberOfOutputEdges; ++numberOfOutputEdges)
				{
					const RenderGraphNodeEdge* edge = node->getOutputEdge(slotIndex, outputIndex);
					RenderGraphNode* nextNode = edge->toNode;
					size_t nextSlot = edge->toSlot;

					const RenderGraphNodeSlotDefinition& slotDef = nextNode->getNodeSlotResourceDefinition(nextSlot);
					VkAccessFlags access = yaptUsageAccessToVkAccess(slotDef.resourceDescription.resourceUsage, slotDef.resourceDescription.accessFlags);
					VkImageLayout layout = yaptUsageToVkImageLayout(slotDef.resourceDescription.resourceUsage, slotDef.resourceDescription.accessFlags);

					stagesAfterSubpass |= yaptShaderStagesToVk(slotDef.resourceDescription.shaderStages);
					accessAfterSubpass |= access;

					assert(layout == finalLayout);
				}

			}

			fillAttachmentDescription(node, slotIndex, initialLayout, finalLayout, attachDesc);
			attachRef.attachment = (uint32_t)attInd;
			attachRef.layout = currentLayout;
		}

		//Simple dependencies, one for before and for after subpass
		subPassDependencies[0].dependencyFlags = 0;
		subPassDependencies[0].dstAccessMask = accessDuringSubpass;
		subPassDependencies[0].srcAccessMask = accessBeforeSubpass;
		subPassDependencies[0].dstStageMask = stagesDuringSubpass;
		subPassDependencies[0].srcStageMask = stagesBeforeSubpass;
		subPassDependencies[0].dstSubpass = 0;
		subPassDependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;

		subPassDependencies[1].dependencyFlags = 0;
		subPassDependencies[1].dstAccessMask = accessAfterSubpass;
		subPassDependencies[1].srcAccessMask = accessDuringSubpass;
		subPassDependencies[1].dstStageMask = stagesAfterSubpass;
		subPassDependencies[1].srcStageMask = stagesDuringSubpass;
		subPassDependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
		subPassDependencies[1].srcSubpass = 0;

		subPassDesc.colorAttachmentCount = (uint32_t)numberOfColorAttachments;
		subPassDesc.pColorAttachments = attachmentReferences.data();
		subPassDesc.pDepthStencilAttachment = hasDepthStencil ? &attachmentReferences.back() : VK_NULL_HANDLE;
		//for now we only have one subpass per renderpass and don't support MS so some of the attachments are always 0
		subPassDesc.inputAttachmentCount = 0;
		subPassDesc.pInputAttachments = VK_NULL_HANDLE;
		subPassDesc.preserveAttachmentCount = 0;
		subPassDesc.pPreserveAttachments = VK_NULL_HANDLE;
		subPassDesc.pResolveAttachments = VK_NULL_HANDLE;

		subPassDesc.flags = 0;
		subPassDesc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;


		vkCreateRenderPass(getGfxApiHandle()->getDevice(), &createInfo, VK_ALLOC_CB, &renderPass);
		return renderPass;
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
				perSlotBarriers.dstStages = getPipelineStageFlags(usageInThisSlot.resourceDescription.resourceUsage, usageInThisSlot.resourceDescription.accessFlags, usageInThisSlot.resourceDescription.shaderStages);

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
					GeneratedBarrierTypeMask generatedBarriersMask = createBarrierIfRequired(usageInThisSlot.resourceDescription.resourceDimensions,
						lastUsage.resourceDescription.resourceUsage, lastUsage.resourceDescription.accessFlags, usageInThisSlot.resourceDescription.resourceUsage, usageInThisSlot.resourceDescription.accessFlags,
						lastUsage.arraySliceOffset, lastUsage.mipOffset, lastUsage.resourceDescription.arraySliceCount, lastUsage.resourceDescription.mipCount, fromQueueFamilyIndex, toQueueFamilyIndex,
						imgBarrier, bufferBarrier, memoryBarrier);

					GeneralPerResourceTransitionInformation& resourceTransitionInfo = m_perResourceBarrierInfo[resID];

					if (generatedBarriersMask & GENERATED_BARRIER_TYPE_IMAGE)
					{
						resourceTransitionInfo.wrapAroundImageBarriers.push_back(imgBarrier);
					}

					if (generatedBarriersMask & GENERATED_BARRIER_TYPE_BUFFER)
					{
						resourceTransitionInfo.wrapAroundBufferBarriers.push_back(bufferBarrier);
					}

					if (generatedBarriersMask & GENERATED_BARRIER_TYPE_MEMORY)
					{
						resourceTransitionInfo.wrapAroundMemoryBarriers.push_back(memoryBarrier);
					}
					AccessFlagsAndLayout lastState;
					lastState.access = yaptUsageAccessToVkAccess(lastUsage.resourceDescription.resourceUsage, lastUsage.resourceDescription.accessFlags);
					lastState.layout = yaptUsageToVkImageLayout(lastUsage.resourceDescription.resourceUsage, lastUsage.resourceDescription.accessFlags);
					resourceTransitionInfo.lastStateInGraph = lastState;

				}
				else
				{

					//TODO: There is a hazard that if the rendertarget is not a single subresource, and different subresources have different layout (several inputs of different layouts), we should be creating barriers to take care of this. for now we assert
					assert((usageInThisSlot.resourceDescription.resourceUsage & (RESOURCE_USAGE_RENDER_TARGET_TEXTURE | RESOURCE_USAGE_DEPTH_TEXTURE | RESOURCE_USAGE_STENCIL_TEXTURE)) == 0 || numberOfInputEdges < 2);

					for (size_t inputIndex = 0; inputIndex != numberOfInputEdges; ++inputIndex)
					{
						const RenderGraphNodeEdge* edge = node->getInputEdge(slotIndex, inputIndex);
						const RenderGraphResourceUsage& previousUsage = m_resourceRequirements.getRenderGraphResourceUsage(edge->fromNode->getSortedIndex(), edge->fromSlot);

						uint32_t fromQueueFamilyIndex = m_queueFamilyIndexPerNode[edge->fromNode->getSortedIndex()];
						//if used in renderpass (or previous usage was renderpass), have renderpass define the dependencies. first usage is an exception since we don't want to create another renderpass if the previous state passed in is different than when RP was created. 
						if (((usageInThisSlot.resourceDescription.resourceUsage | previousUsage.resourceDescription.resourceUsage) & (RESOURCE_USAGE_RENDER_TARGET_TEXTURE | RESOURCE_USAGE_DEPTH_TEXTURE | RESOURCE_USAGE_STENCIL_TEXTURE)) != 0)
						{
							continue;
						}

						GeneratedBarrierTypeMask generatedBarriersMask = createBarrierIfRequired(usageInThisSlot.resourceDescription.resourceDimensions,
							previousUsage.resourceDescription.resourceUsage, previousUsage.resourceDescription.accessFlags, usageInThisSlot.resourceDescription.resourceUsage, usageInThisSlot.resourceDescription.accessFlags,
							previousUsage.arraySliceOffset, previousUsage.mipOffset, previousUsage.resourceDescription.arraySliceCount, previousUsage.resourceDescription.mipCount, fromQueueFamilyIndex, toQueueFamilyIndex,
							imgBarrier, bufferBarrier, memoryBarrier);
						
						if (generatedBarriersMask & GENERATED_BARRIER_TYPE_IMAGE)
						{
							perSlotBarriers.imageBarriers.push_back(imgBarrier);
						}

						if (generatedBarriersMask & GENERATED_BARRIER_TYPE_BUFFER)
						{
							perSlotBarriers.bufferBarriers.push_back(bufferBarrier);
						}

						if (generatedBarriersMask & GENERATED_BARRIER_TYPE_MEMORY)
						{
							perSlotBarriers.memoryBarriers.push_back(memoryBarrier);
						}

						perSlotBarriers.srcStages |= getPipelineStageFlags(previousUsage.resourceDescription.resourceUsage, previousUsage.resourceDescription.accessFlags, previousUsage.resourceDescription.shaderStages);
						
					}
				}
			}
		}

		
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
		assert(!"TODO");
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

	VkPipelineStageFlags RenderGraphVk::getPipelineStageFlags(ResourceUsage resourceUsage, AccessFlags accessFlags, ShaderStages shaderStages)
	{
		VkPipelineStageFlags flags = 0;
		FLAGS_CONVERT(resourceUsage, flags, RESOURCE_USAGE_COPY_DESTINATION | RESOURCE_USAGE_COPY_SOURCE, VK_PIPELINE_STAGE_TRANSFER_BIT);
		FLAGS_CONVERT(resourceUsage, flags, RESOURCE_USAGE_VERTEX_BUFFER | RESOURCE_USAGE_INDEX_BUFFER, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT);
		FLAGS_CONVERT(resourceUsage, flags, RESOURCE_USAGE_RENDER_TARGET_TEXTURE, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);

		if ((resourceUsage & RESOURCE_USAGE_DEPTH_STENCIL_TEXTURE) != 0)
		{
			if (accessFlags & ACCESS_FLAGS_WRITE)
			{
				flags |= VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
			}

			if (accessFlags & ACCESS_FLAGS_READ)
			{
				flags |= VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
			}
		}
		// shader resources
		if ((resourceUsage & (RESOURCE_USAGE_UNIFORM_BUFFER | 
			RESOURCE_USAGE_UNIFORM_TEXEL_BUFFER |
			RESOURCE_USAGE_STORAGE_BUFFER | 
			RESOURCE_USAGE_STORAGE_TEXEL_BUFFER | 
			RESOURCE_USAGE_SAMPLED_TEXTURE | 
			RESOURCE_USAGE_STORAGE_TEXTURE |
			RESOURCE_USAGE_ACCELERATION_STRUCTURE_BUFFER |
			RESOURCE_USAGE_SHADERTABLE_BUFFER)) != 0)
		{
			FLAGS_CONVERT(shaderStages, flags, SHADERSTAGE_VERTEX, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT);
			FLAGS_CONVERT(shaderStages, flags, SHADERSTAGE_HULL, VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT);
			FLAGS_CONVERT(shaderStages, flags, SHADERSTAGE_DOMAIN, VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT);
			FLAGS_CONVERT(shaderStages, flags, SHADERSTAGE_GEOMETRY, VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT);
			FLAGS_CONVERT(shaderStages, flags, SHADERSTAGE_FRAGMENT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
			FLAGS_CONVERT(shaderStages, flags, SHADERSTAGE_COMPUTE, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);

			FLAGS_CONVERT(shaderStages, flags, SHADERSTAGE_RT_RAYGENERATION | SHADERSTAGE_RT_MISS | SHADERSTAGE_RT_ANY_HIT | SHADERSTAGE_RT_CLOSEST_HIT | SHADERSTAGE_RT_INTERSECTION, VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR);
		}

		return flags;
	}

	bool RenderGraphVk::getVkAccessMaskTransition(ResourceUsage usageFrom, AccessFlags accessFlagsFrom, ResourceUsage usageTo, AccessFlags accessFlagsTo, VkAccessFlags& from, VkAccessFlags& to)
	{
		if (usageFrom == usageTo && accessFlagsFrom == accessFlagsTo) return false;
		from = yaptUsageAccessToVkAccess(usageFrom, accessFlagsFrom);
		to = yaptUsageAccessToVkAccess(usageTo, accessFlagsTo);

		return true;

	}
	bool RenderGraphVk::getVkImageLayoutTransition(ResourceUsage usageFrom, AccessFlags accessFlagsFrom, ResourceUsage usageTo, AccessFlags accessFlagsTo, VkImageLayout& from, VkImageLayout& to)
	{
		if (usageFrom == usageTo && accessFlagsFrom == accessFlagsTo) return false;
		from = yaptUsageToVkImageLayout(usageFrom, accessFlagsFrom);
		to = yaptUsageToVkImageLayout(usageTo, accessFlagsTo);
		return true;
	}

	RenderGraphVk::GeneratedBarrierTypeMask RenderGraphVk::createBarrierIfRequired(ResourceDimension resDimension, ResourceUsage usageFrom, AccessFlags accessFlagsFrom, ResourceUsage usageTo, AccessFlags accessFlagsTo,
		uint32_t arrayOffset, uint32_t mipOffset, uint32_t arrayCount, uint32_t mipCount, uint32_t srcQueueFamilyIndex, uint32_t dstQueueFamilyIndex,
		VkImageMemoryBarrier& imageBarrierOut, VkBufferMemoryBarrier& bufferBarrierOut, VkMemoryBarrier& memoryBarrier)
	{
		VkAccessFlags fromAccess;
		VkAccessFlags toAccess;
		RenderGraphVk::GeneratedBarrierTypeMask mask = GENERATED_BARRIER_TYPE_NONE;

		bool needsStorageHazardBarrier = false;
		if ((usageTo & (RESOURCE_USAGE_STORAGE_BUFFER | RESOURCE_USAGE_STORAGE_TEXEL_BUFFER | RESOURCE_USAGE_STORAGE_TEXTURE)) != 0 && ((accessFlagsFrom | accessFlagsTo) & ACCESS_FLAGS_WRITE) != 0)
		{
			needsStorageHazardBarrier = true;
		}
		

		if (getVkAccessMaskTransition(usageFrom, accessFlagsFrom, usageTo, accessFlagsTo, fromAccess, toAccess) || needsStorageHazardBarrier)
		{
			switch (resDimension)
			{
			case ResourceDimension::BUFFER:
			{
				bufferBarrierOut.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
				bufferBarrierOut.pNext = NULL;
				bufferBarrierOut.srcQueueFamilyIndex = srcQueueFamilyIndex;
				bufferBarrierOut.dstQueueFamilyIndex = dstQueueFamilyIndex;

				bufferBarrierOut.srcAccessMask = fromAccess;
				bufferBarrierOut.dstAccessMask = toAccess;

				bufferBarrierOut.offset = 0;
				bufferBarrierOut.size = 0;
				bufferBarrierOut.buffer = VK_NULL_HANDLE;

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

				imageBarrierOut.subresourceRange.aspectMask = yaptUsageToAspectFlags(usageFrom | usageTo);
				imageBarrierOut.subresourceRange.baseArrayLayer = arrayOffset;
				imageBarrierOut.subresourceRange.baseMipLevel = mipOffset;
				imageBarrierOut.subresourceRange.layerCount = arrayCount;
				imageBarrierOut.subresourceRange.levelCount = mipCount;

				mask |= GENERATED_BARRIER_TYPE_IMAGE;
			}

			break;

			default:
				assert(!"not implemented/unknown");
			}
		}
		return mask;
		
	}
	
	
}