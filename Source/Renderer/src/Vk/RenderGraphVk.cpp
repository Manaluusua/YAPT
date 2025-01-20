#include <Renderer/Vk/RenderGraphVk.h>

#include <Renderer/Vk/RendererVk.h>
#include <Renderer/Vk/ResourceManagerVk.h>

#include <Renderer/Vk/RenderNodeVk.h>
#include <Renderer/Vk/ComputeNodeVk.h>
#include <Renderer/Vk/RaytraceNodeVk.h>
#include <Renderer/Vk/ResourceHandlesVk.h>

#include <array>

#include <Renderer/Vk/YaptToVkConversions.h>


#if defined(DEBUG) || defined(_DEBUG) 
#define VERBOSE_BARRIER_MERGE_VK
#endif

namespace YAPT
{
	class SwapChainNodeVk : public SwapChainNode
	{
	public:

		SwapChainNodeVk(const char* name, RenderGraph* graph, RenderGraphNodeSlotDefinition* definitions, size_t definitionCount)
			:SwapChainNode(name, graph, definitions, definitionCount)
		{

		}

		virtual ~SwapChainNodeVk()
		{
		}


	};

	RenderGraphVk::RenderGraphVk(GfxApiHandle h)
		:RenderGraph(h)
	{
		m_queueTransitionHelper.initialize(h->getDevice(), h->getFramePipelineLength(), 1);
	}
	RenderGraphVk::~RenderGraphVk()
	{
		m_queueTransitionHelper.deinitialize();
		for (size_t i = 0; i < m_frameBuffers.size(); ++i)
		{
			m_gfxHandle->getResourceManager()->deferredDestroyVkResource(m_frameBuffers[i]);
		}

		for (size_t i = 0; i < m_renderPasses.size(); ++i)
		{
			m_gfxHandle->getResourceManager()->deferredDestroyVkResource(m_renderPasses[i]);
		}
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

	SwapChainNode* RenderGraphVk::createSwapChainNodeInternal(const char* name)
	{
		RenderGraphNodeSlotDefinition slotDef = { ResourceDimension::TEXTURE_2D,
			m_gfxHandle->getSwapChainFormat(),
			RESOURCE_USAGE_PRESENTABLE_TEXTURE,
			ACCESS_FLAGS_READ,
			SHADERSTAGE_NONE,
			1,
			1
		};

		return new SwapChainNodeVk(name, this, &slotDef, 1);
	}

	void RenderGraphVk::issueBarriers(size_t nodeIndex, CommandBufferHandle buffer)
	{
		BarriersPerNode& barriersPerNode = m_barriers[nodeIndex];

		
		std::vector<VkMemoryBarrier>& memBarriersCache = barriersPerNode.memBarriersCache;
		std::vector<VkBufferMemoryBarrier>& bufBarriersCache = barriersPerNode.bufBarriersCache;
		std::vector<VkImageMemoryBarrier>& imgBarriersCache = barriersPerNode.imgBarriersCache;
		
		memBarriersCache.clear();
		bufBarriersCache.clear();
		imgBarriersCache.clear();

		VkPipelineStageFlags srcStages = 0;
		VkPipelineStageFlags dstStages = 0;

		auto addBarriers = [&memBarriersCache, &bufBarriersCache, &imgBarriersCache](std::vector<VkMemoryBarrier>& memoryBarriers, std::vector<VkBufferMemoryBarrier>& bufferBarriers, std::vector<VkImageMemoryBarrier>& imageBarriers)
		{
			size_t memBarrierOffset = memBarriersCache.size();
			size_t imgBarrierOffset = imgBarriersCache.size();
			size_t bufBarrierOffset = bufBarriersCache.size();

			memBarriersCache.resize(memBarriersCache.size() + memoryBarriers.size());
			bufBarriersCache.resize(bufBarriersCache.size() + bufferBarriers.size());
			imgBarriersCache.resize(imgBarriersCache.size() + imageBarriers.size());

			std::copy(memoryBarriers.data(), memoryBarriers.data() + memoryBarriers.size(), memBarriersCache.data() + memBarrierOffset);
			std::copy(imageBarriers.data(), imageBarriers.data() + imageBarriers.size(), imgBarriersCache.data() + imgBarrierOffset);
			std::copy(bufferBarriers.data(), bufferBarriers.data() + bufferBarriers.size(), bufBarriersCache.data() + bufBarrierOffset);
		};

		for (size_t i = 0; i < barriersPerNode.perSlotDesc.size(); ++i)
		{
			ResourceSlotBarrierDescription& barrierSlotDesc = barriersPerNode.perSlotDesc[i];
			if (!barrierSlotDesc.hasValidBarriers) continue;

			srcStages |= barrierSlotDesc.srcStages;
			dstStages |= barrierSlotDesc.dstStages;

			if (barrierSlotDesc.isFirstUsageForResource)
			{
				size_t resourceIndex = m_resourceRequirements.getRenderGraphResourceIndex(nodeIndex, i);
				GeneralPerResourceTransitionInformation& info = m_perResourceBarrierInfo[resourceIndex];

				if (info.useWrapAroundBarriers)
				{
					addBarriers(barrierSlotDesc.memoryBarriersEveryFrame, barrierSlotDesc.bufferBarriersEveryFrame, barrierSlotDesc.imageBarriersEveryFrame);
				}
				else
				{
					addBarriers(barrierSlotDesc.memoryBarriersOnce, barrierSlotDesc.bufferBarriersOnce, barrierSlotDesc.imageBarriersOnce);
					info.useWrapAroundBarriers = true;
				}
			}
			else
			{
				addBarriers(barrierSlotDesc.memoryBarriersEveryFrame, barrierSlotDesc.bufferBarriersEveryFrame, barrierSlotDesc.imageBarriersEveryFrame);
			}

		}

		bool hasBarrierstoIssue = memBarriersCache.size() > 0 || bufBarriersCache.size() > 0 || imgBarriersCache.size() > 0;

		if (hasBarrierstoIssue)
		{
			vkCmdPipelineBarrier(buffer, srcStages, dstStages, 0, (uint32_t)memBarriersCache.size(), memBarriersCache.data(), (uint32_t)bufBarriersCache.size(), bufBarriersCache.data(), (uint32_t)imgBarriersCache.size(), imgBarriersCache.data());
		}
	}



	void RenderGraphVk::prepareNodeExecution(RenderNodeVk* node, const RenderGraphNodeExecutionContext& context)
	{
		bool needsNewFrameBuffer = false;
		bool hasDepthStencil = node->hasDepthStencil();
		size_t numberOfColorTargets = node->getNumberOfColorTargets();
		size_t frameBufferRenderPassIndex = node->getRenderGraphInternalRenderNodeResourcesIndex();
		size_t totalNumberOfAttachments = numberOfColorTargets + (hasDepthStencil ? 1 : 0);
		VkDevice device = m_gfxHandle->getDevice();

		uint32_t width = 0;
		uint32_t height = 0;

		for (size_t attInd = 0; attInd < totalNumberOfAttachments; ++attInd)
		{
			bool isDepthStencilAttachment = attInd >= numberOfColorTargets;
			size_t slotIndex = isDepthStencilAttachment ? node->getNodeSlotIndexForDepthStencil() : node->getNodeSlotIndexForRenderTargetIndex(attInd);
			const RenderGraphResourceId resId = getRenderGraphResourceIdUsedInSlot(node->getSortedIndex(), slotIndex);
			const RenderGraphResourceDescription& resDesc = getRenderGraphResourceDescription(resId);

			if(isResourceBoundThisFrame(resId))
			{
				needsNewFrameBuffer = true;
			}

			assert(m_boundRenderGraphResources[resId].textureHandles.size() == 1 && "Binding more than one resource per slot is not allowed for rendertargets!");

			const TextureHandle& texHandle = m_boundRenderGraphResources[resId].textureHandles[0];

			if (attInd == 0)
			{
				width = texHandle->createInfo.extent.width;
				height = texHandle->createInfo.extent.height;

			}
			else
			{
				assert(texHandle->createInfo.extent.width == width && "Renderpass attachment dimensions don't match!");
				assert(texHandle->createInfo.extent.height == height && "Renderpass attachment dimensions don't match!");

			}
		}


		if (needsNewFrameBuffer)
		{
			if (m_frameBuffers[frameBufferRenderPassIndex] != VK_NULL_HANDLE)
			{
				m_gfxHandle->getResourceManager()->deferredDestroyVkResource(m_frameBuffers[frameBufferRenderPassIndex]);
				m_frameBuffers[frameBufferRenderPassIndex] = VK_NULL_HANDLE;
			}

			m_frameBuffers[frameBufferRenderPassIndex] = createFrameBuffer(node);
		}

		VkRenderPassBeginInfo renderPassBeginInfo = {};
		renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassBeginInfo.renderPass = m_renderPasses[frameBufferRenderPassIndex];
		renderPassBeginInfo.framebuffer = m_frameBuffers[frameBufferRenderPassIndex];
		renderPassBeginInfo.renderArea = { {0, 0}, {width, height} };
		renderPassBeginInfo.pClearValues = node->getClearValues();
		renderPassBeginInfo.clearValueCount = node->getClearValueCount();

		vkCmdBeginRenderPass(context.cmdBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
	}
	void RenderGraphVk::prepareNodeExecution(ComputeNodeVk* node, const RenderGraphNodeExecutionContext& context)
	{
		
	}
	void RenderGraphVk::prepareNodeExecution(RaytraceNodeVk* node, const RenderGraphNodeExecutionContext& context)
	{

	}

	void RenderGraphVk::endNodeExecution(RenderNodeVk* node, const RenderGraphNodeExecutionContext& context)
	{
		vkCmdEndRenderPass(context.cmdBuffer);
	}
	void RenderGraphVk::endNodeExecution(ComputeNodeVk* node, const RenderGraphNodeExecutionContext& context)
	{

	}
	void RenderGraphVk::endNodeExecution(RaytraceNodeVk* node, const RenderGraphNodeExecutionContext& context)
	{

	}


	void RenderGraphVk::beginExecution()
	{
		VkSemaphore waitSemaphore = m_gfxHandle->getResourceManager()->getLastSignaledSemaphore();
		VkSemaphore semaphoreOut;
		m_queueTransitionHelper.issueTransitionBarriers(m_gfxHandle->getSubmissionThread(), m_gfxHandle->getFramePipelineIndex(), 0, &waitSemaphore, 1, semaphoreOut);
		if (semaphoreOut != VK_NULL_HANDLE)
		{
			m_gfxHandle->getResourceManager()->overrideLastSignaledSemaphore(semaphoreOut);
		}
		
	}
	void RenderGraphVk::endExecution()
	{

	}
	void RenderGraphVk::afterRenderGraphSubmit()
	{
		RenderGraph::afterRenderGraphSubmit();
		m_queueTransitionHelper.clearBarriers();
	}


	void RenderGraphVk::executeNodesInternal(RenderGraphNode** nodes, size_t nodeCount, const RenderGraphNodeExecutionContext& context)
	{

		
		for (size_t i = 0; i < nodeCount; ++i)
		{
			size_t nodeIndex = nodes[i]->getSortedIndex();
			issueBarriers(nodeIndex, context.cmdBuffer);

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


			invokeNodeCallback(nodes[i],context);

			switch (nodes[i]->getType())
			{
			case RenderGraphNode::Type::COMPUTE:
				endNodeExecution(static_cast<ComputeNodeVk*>(nodes[i]), context);
				break;
			case RenderGraphNode::Type::RENDER:
				endNodeExecution(static_cast<RenderNodeVk*>(nodes[i]), context);
				break;
			case RenderGraphNode::Type::RAYTRACE:
				endNodeExecution(static_cast<RaytraceNodeVk*>(nodes[i]), context);
				break;
			}
		}

	}

	void RenderGraphVk::fillAttachmentDescription(RenderNode* node, size_t slot, VkImageLayout initialLayout, VkImageLayout finalLayout, VkAttachmentDescription& descOut) const
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
			descOut.loadOp = loadOp;
			descOut.storeOp = storeOp;
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

	VkFramebuffer RenderGraphVk::createFrameBuffer(RenderNodeVk* node) const
	{
		const uint32_t MAX_IMAGE_VIEWS = 16;
		std::array<VkImageView, MAX_IMAGE_VIEWS> imageViews;

		bool needsNewFrameBuffer = false;
		bool hasDepthStencil = node->hasDepthStencil();
		size_t numberOfColorTargets = node->getNumberOfColorTargets();
		size_t totalNumberOfAttachments = numberOfColorTargets + (hasDepthStencil ? 1 : 0);
		size_t frameBufferRenderPassIndex = node->getRenderGraphInternalRenderNodeResourcesIndex();
		VkDevice device = m_gfxHandle->getDevice();

		uint32_t width = 0;
		uint32_t height = 0;
		uint32_t layers = 0;

		for (size_t attInd = 0; attInd < totalNumberOfAttachments; ++attInd)
		{
			bool isDepthStencilAttachment = attInd >= numberOfColorTargets;
			size_t slotIndex = isDepthStencilAttachment ? node->getNodeSlotIndexForDepthStencil() : node->getNodeSlotIndexForRenderTargetIndex(attInd);
			const RenderGraphResourceId resId = getRenderGraphResourceIdUsedInSlot(node->getSortedIndex(), slotIndex);
			const RenderGraphResourceView& view = m_resourceDataPerNodeSlot[node->getSortedIndex()].resourceViewPerSlot[slotIndex];
			const RenderGraphResourceDescription& resDesc = getRenderGraphResourceDescription(resId);

			assert(view.textureViews.size() == 1 && "Binding more than one resource per slot is not allowed for rendertargets!");
			assert(m_boundRenderGraphResources[resId].textureHandles.size() == 1 && "Binding more than one resource per slot is not allowed for rendertargets!");

			const TextureHandle& texHandle = m_boundRenderGraphResources[resId].textureHandles[0];
			
			if (attInd == 0)
			{
				width = texHandle->createInfo.extent.width;
				height = texHandle->createInfo.extent.height;
				layers = resDesc.arraySliceCount;
			}
			else
			{
				assert(texHandle->createInfo.extent.width == width && "Framebuffer attachment dimensions don't match!");
				assert(texHandle->createInfo.extent.height == height && "Framebuffer attachment dimensions don't match!");
				assert(resDesc.arraySliceCount == layers && "Framebuffer attachment dimensions don't match!");
			}
			
			

			imageViews[attInd] = view.textureViews[0];
		}

		VkFramebufferCreateInfo fbCreateInfo = {};
		fbCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		fbCreateInfo.pNext = nullptr;
		fbCreateInfo.flags = 0;

		fbCreateInfo.renderPass = m_renderPasses[frameBufferRenderPassIndex];
		fbCreateInfo.attachmentCount = (uint32_t)totalNumberOfAttachments;
		fbCreateInfo.pAttachments = imageViews.data();
		fbCreateInfo.width = width;
		fbCreateInfo.height = height;
		fbCreateInfo.layers = layers;

		VkFramebuffer fb;
		checkForVkError(vkCreateFramebuffer(device, &fbCreateInfo, VK_ALLOC_CB, &fb));
		return fb;

	}

	VkRenderPass RenderGraphVk::createRenderPass(RenderNode* node) const
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

		//if there are no attachments for a subpass that needs a dependency (either all inputs are first use where a barrier handles the sync, and/or all attachments are last use, where a barrier in the next first use handles the depdendency) 
		uint32_t subPassDepsCount = 2;
		uint32_t subPassDepsOffset = 0;
		if (stagesBeforeSubpass == 0)
		{
			--subPassDepsCount;
			++subPassDepsOffset;
		}
		if (stagesAfterSubpass == 0)
		{
			--subPassDepsCount;
		}

		createInfo.dependencyCount = subPassDepsCount;
		createInfo.pDependencies = &subPassDependencies[subPassDepsOffset];

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


		VkResult res = vkCreateRenderPass(getGfxApiHandle()->getDevice(), &createInfo, VK_ALLOC_CB, &renderPass);
		assert(res == VK_SUCCESS);
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
				rNode->setRenderGraphResourcesIndex((uint32_t)rpIndex);
				rNode->setRenderPass(rp, 0);
			}

		}

		//framebuffer per renderpass
		m_frameBuffers.resize(m_renderPasses.size(), VK_NULL_HANDLE);

		std::vector<VkImageLayout> imageLayouts;
		imageLayouts.reserve(512);

		for (size_t nodeIndex = 0; nodeIndex != nodeCount; ++nodeIndex)
		{
			BarriersPerNode& perNodeBarriers = m_barriers[nodeIndex];
			perNodeBarriers.perSlotDesc.resize(getNodes()[nodeIndex]->getNumberOfSlots());
		}

		m_perResourceBarrierInfo.resize(m_resourceRequirements.getNumberOfRenderGraphResourceDescriptions());

		for (size_t resourceIndex = 0; resourceIndex < m_resourceRequirements.getNumberOfRenderGraphResourceDescriptions(); ++resourceIndex)
		{
			const RenderGraphResourceDescription& resourceDesc = m_resourceRequirements.getRenderGraphResourceDescription(resourceIndex);

			imageLayouts.assign(size_t(resourceDesc.mipCount * resourceDesc.arraySliceCount), VK_IMAGE_LAYOUT_UNDEFINED);
			

			const NodeSlotIdentifier* nodeSlotIdentifiers;
			size_t numberOfNodeSlotIdentifiers;
			m_resourceRequirements.getNodeSlotsUsingResource(resourceIndex, nodeSlotIdentifiers, numberOfNodeSlotIdentifiers);

			for (size_t resourceUsageIndex = 0; resourceUsageIndex < numberOfNodeSlotIdentifiers; ++resourceUsageIndex)
			{
				size_t nodeIndex = nodeSlotIdentifiers[resourceUsageIndex].sortedNodeIndex;
				size_t slotIndex = nodeSlotIdentifiers[resourceUsageIndex].slotIndex;

				BarriersPerNode& perNodeBarriers = m_barriers[nodeIndex];
				RenderGraphNode* node = getNodes()[nodeIndex];

				uint32_t toQueueFamilyIndex = m_queueFamilyIndexPerNode[node->getSortedIndex()];


				ResourceSlotBarrierDescription& perSlotBarriers = perNodeBarriers.perSlotDesc[slotIndex];
				const RenderGraphResourceUsage& usageInThisSlot = m_resourceRequirements.getRenderGraphResourceUsage(nodeIndex, slotIndex);

				perSlotBarriers.srcStages = 0;
				perSlotBarriers.dstStages = getPipelineStageFlags(usageInThisSlot.resourceDescription.resourceUsage, usageInThisSlot.resourceDescription.accessFlags, usageInThisSlot.resourceDescription.shaderStages);
				perSlotBarriers.hasValidBarriers = true;

				size_t numberOfInputEdges = node->getNumberOfInputEdges(slotIndex);

				bool isFirstUsage = numberOfInputEdges == 0;
				perSlotBarriers.isFirstUsageForResource = isFirstUsage;

				VkImageMemoryBarrier imgBarrier;
				VkBufferMemoryBarrier bufferBarrier;
				VkMemoryBarrier memoryBarrier;

				if (isFirstUsage)
				{
					size_t lastUsedNode = nodeSlotIdentifiers[numberOfNodeSlotIdentifiers - 1].sortedNodeIndex;
					size_t lastUsedSlot = nodeSlotIdentifiers[numberOfNodeSlotIdentifiers - 1].slotIndex;

					const RenderGraphResourceUsage& lastUsage = m_resourceRequirements.getRenderGraphResourceUsage(lastUsedNode, lastUsedSlot);
					uint32_t fromQueueFamilyIndex = m_queueFamilyIndexPerNode[lastUsedNode];
					GeneratedBarrierTypeMask generatedBarriersMask = createBarrierIfRequired(usageInThisSlot.resourceDescription.resourceDimensions,
						lastUsage.resourceDescription.resourceUsage, lastUsage.resourceDescription.accessFlags, usageInThisSlot.resourceDescription.resourceUsage, usageInThisSlot.resourceDescription.accessFlags,
						lastUsage.arraySliceOffset, lastUsage.mipOffset, lastUsage.resourceDescription.arraySliceCount, lastUsage.resourceDescription.mipCount, fromQueueFamilyIndex, toQueueFamilyIndex,
						imgBarrier, bufferBarrier, memoryBarrier);

					GeneralPerResourceTransitionInformation& resourceTransitionInfo = m_perResourceBarrierInfo[resourceIndex];

					if (generatedBarriersMask & GENERATED_BARRIER_TYPE_IMAGE)
					{
						resourceTransitionInfo.wrapAroundImageBarriers.push_back(imgBarrier);

						for (uint32_t arraySlice = 0; arraySlice < usageInThisSlot.resourceDescription.arraySliceCount; ++arraySlice)
						{
							for (uint32_t mipIndex = 0; mipIndex < usageInThisSlot.resourceDescription.mipCount; ++mipIndex)
							{
								uint32_t subResourceIndex = calculateSubresourceIndex(mipIndex + usageInThisSlot.mipOffset, arraySlice + usageInThisSlot.arraySliceOffset, resourceDesc.mipCount, resourceDesc.arraySliceCount);
								imageLayouts[subResourceIndex] = imgBarrier.newLayout;
							}
						}
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

					perSlotBarriers.srcStages |= getPipelineStageFlags(lastUsage.resourceDescription.resourceUsage, lastUsage.resourceDescription.accessFlags, lastUsage.resourceDescription.shaderStages);//VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
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
							perSlotBarriers.hasValidBarriers = false;
							continue;
						}

						GeneratedBarrierTypeMask generatedBarriersMask = createBarrierIfRequired(usageInThisSlot.resourceDescription.resourceDimensions,
							previousUsage.resourceDescription.resourceUsage, previousUsage.resourceDescription.accessFlags, usageInThisSlot.resourceDescription.resourceUsage, usageInThisSlot.resourceDescription.accessFlags,
							previousUsage.arraySliceOffset, previousUsage.mipOffset, previousUsage.resourceDescription.arraySliceCount, previousUsage.resourceDescription.mipCount, fromQueueFamilyIndex, toQueueFamilyIndex,
							imgBarrier, bufferBarrier, memoryBarrier);

						if (generatedBarriersMask & GENERATED_BARRIER_TYPE_IMAGE)
						{
							//because different nodes can use same resource as readonly without having dependency, need to track the actual layout (we might have already transitioned to the correct layout in some previous node which also used this resource as readonly)
							VkImageLayout previousLayout = VK_IMAGE_LAYOUT_UNDEFINED;
							for (uint32_t arraySlice = 0; arraySlice < previousUsage.resourceDescription.arraySliceCount; ++arraySlice)
							{
								for (uint32_t mipIndex = 0; mipIndex < previousUsage.resourceDescription.mipCount; ++mipIndex)
								{
									uint32_t subResourceIndex = calculateSubresourceIndex(mipIndex + previousUsage.mipOffset, arraySlice + previousUsage.arraySliceOffset, resourceDesc.mipCount, resourceDesc.arraySliceCount);
									if (previousLayout == VK_IMAGE_LAYOUT_UNDEFINED)
									{
										previousLayout = imageLayouts[subResourceIndex];
									}
									else
									{
										assert(previousLayout == imageLayouts[subResourceIndex]); //all the subresources from previous stage should be in the same layout
									}
									imageLayouts[subResourceIndex] = imgBarrier.newLayout;
								}
							}
							imgBarrier.oldLayout = previousLayout;
							perSlotBarriers.preGeneratedImageBarriers.push_back(imgBarrier);
						}

						if (generatedBarriersMask & GENERATED_BARRIER_TYPE_BUFFER)
						{
							perSlotBarriers.preGeneratedBufferBarriers.push_back(bufferBarrier);
						}

						if (generatedBarriersMask & GENERATED_BARRIER_TYPE_MEMORY)
						{
							perSlotBarriers.preGeneratedMemoryBarriers.push_back(memoryBarrier);
						}

						//TODO: if the previous usage and current usage are from different queue families, need both release and acquire barriers. Ignored for now.
						assert(fromQueueFamilyIndex == toQueueFamilyIndex);

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



	void RenderGraphVk::resourcesBoundToPipeline(RenderGraphResourceId id, size_t numberOfResourcesBound)
	{
		updateBarriersForResource(id,  numberOfResourcesBound);
	}

	
	void RenderGraphVk::updateBarriersForResource(RenderGraphResourceId id, size_t numberOfResourcesBound)
	{
		const NodeSlotIdentifier* nodeSlotIdentifiers;
		size_t numberOfNodeSlotIdentifiers;
		m_resourceRequirements.getNodeSlotsUsingResource(id, nodeSlotIdentifiers, numberOfNodeSlotIdentifiers);

		GeneralPerResourceTransitionInformation& resourceTransitionInfo = m_perResourceBarrierInfo[id];
		resourceTransitionInfo.useWrapAroundBarriers = false;

		for (size_t i = 0; i < numberOfNodeSlotIdentifiers; ++i)
		{
			size_t nodeIndex = nodeSlotIdentifiers[i].sortedNodeIndex;
			size_t slotIndex = nodeSlotIdentifiers[i].slotIndex;
			ResourceSlotBarrierDescription& barrierDescs = m_barriers[nodeIndex].perSlotDesc[slotIndex];
			uint32_t toQueueFamilyIndex = m_queueFamilyIndexPerNode[nodeIndex];

			//copy per resource barriers to current barriers (the amount of barriers could potentially change because of more/less resources bound to slot)

			size_t preGeneratedMemoryBarrierCount = barrierDescs.preGeneratedMemoryBarriers.size();
			size_t preGeneratedImageBarrierCount = barrierDescs.preGeneratedImageBarriers.size();
			size_t preGeneratedBufferBarrierCount = barrierDescs.preGeneratedBufferBarriers.size();

			size_t extraMemoryBarriersCount = 0;
			size_t extraImageBarriersCount = 0;
			size_t extraBufferBarriersCount = 0;

			if (barrierDescs.isFirstUsageForResource)
			{
				extraMemoryBarriersCount = resourceTransitionInfo.wrapAroundMemoryBarriers.size();
				extraImageBarriersCount = resourceTransitionInfo.wrapAroundImageBarriers.size();
				extraBufferBarriersCount = resourceTransitionInfo.wrapAroundBufferBarriers.size();
			}

			barrierDescs.memoryBarriersEveryFrame.resize((preGeneratedMemoryBarrierCount + extraMemoryBarriersCount) * numberOfResourcesBound);
			barrierDescs.imageBarriersEveryFrame.resize((preGeneratedImageBarrierCount + extraImageBarriersCount) * numberOfResourcesBound);
			barrierDescs.bufferBarriersEveryFrame.resize((preGeneratedBufferBarrierCount + extraBufferBarriersCount) * numberOfResourcesBound);
			

			barrierDescs.memoryBarriersOnce.resize(preGeneratedMemoryBarrierCount * numberOfResourcesBound);
			barrierDescs.imageBarriersOnce.resize(preGeneratedImageBarrierCount * numberOfResourcesBound);
			barrierDescs.bufferBarriersOnce.resize(preGeneratedBufferBarrierCount * numberOfResourcesBound);
			

			for (size_t resIndex = 0; resIndex < numberOfResourcesBound; ++resIndex)
			{

				if (m_boundRenderGraphResources[id].type == BoundResourceType::TEXTURE)
				{
					size_t imageBarriersPerResource = barrierDescs.preGeneratedImageBarriers.size();
					VkImage imageHandle = m_boundRenderGraphResources[id].textureHandles[resIndex]->image;
					for (size_t barrierIndex = 0; barrierIndex < imageBarriersPerResource; ++barrierIndex)
					{
						size_t dstBarrierIndex = resIndex * imageBarriersPerResource + barrierIndex;
						barrierDescs.imageBarriersEveryFrame[dstBarrierIndex] = barrierDescs.preGeneratedImageBarriers[barrierIndex];
						barrierDescs.imageBarriersEveryFrame[dstBarrierIndex].image = imageHandle;
					}
				}
				else if (m_boundRenderGraphResources[id].type == BoundResourceType::BUFFER)
				{
					size_t bufferBarriersPerResource = barrierDescs.preGeneratedBufferBarriers.size();
					VkBuffer buffer = m_boundRenderGraphResources[id].bufferHandles[resIndex]->buffer;
					for (size_t barrierIndex = 0; barrierIndex < bufferBarriersPerResource; ++barrierIndex)
					{
						size_t dstBarrierIndex = resIndex * bufferBarriersPerResource + barrierIndex;
						barrierDescs.bufferBarriersEveryFrame[dstBarrierIndex] = barrierDescs.preGeneratedBufferBarriers[barrierIndex];
						barrierDescs.bufferBarriersEveryFrame[dstBarrierIndex].buffer = buffer;
					}
				}
				else
				{
					assert(!"unknown binding");
				}

			}

			//if this is the first usage for a resource, need to copy the above barriers to be executed on the first time running the graph + barriers to transition to whatever "state" is needed.
			//furthermore, append "wrap around" barriers from last state on the graph to what is needed as first state
			if (barrierDescs.isFirstUsageForResource)
			{
				std::copy(barrierDescs.memoryBarriersEveryFrame.data(), barrierDescs.memoryBarriersEveryFrame.data() + barrierDescs.memoryBarriersOnce.size(), barrierDescs.memoryBarriersOnce.data());
				std::copy(barrierDescs.imageBarriersEveryFrame.data(), barrierDescs.imageBarriersEveryFrame.data() + barrierDescs.imageBarriersOnce.size(), barrierDescs.imageBarriersOnce.data());
				std::copy(barrierDescs.bufferBarriersEveryFrame.data(), barrierDescs.bufferBarriersEveryFrame.data() + barrierDescs.bufferBarriersOnce.size(), barrierDescs.bufferBarriersOnce.data());


				//append wrap around barriers to "every frame" barriers
				for (size_t resIndex = 0; resIndex < numberOfResourcesBound; ++resIndex)
				{

					if (m_boundRenderGraphResources[id].type == BoundResourceType::TEXTURE)
					{
						size_t wrapAroundBarriersPerResource = resourceTransitionInfo.wrapAroundImageBarriers.size();
						VkImage imageHandle = m_boundRenderGraphResources[id].textureHandles[resIndex]->image;
						for (size_t barrierIndex = 0; barrierIndex < wrapAroundBarriersPerResource; ++barrierIndex)
						{
							size_t dstBarrierIndex = preGeneratedImageBarrierCount * numberOfResourcesBound + resIndex * wrapAroundBarriersPerResource + barrierIndex;
							barrierDescs.imageBarriersEveryFrame[dstBarrierIndex] = resourceTransitionInfo.wrapAroundImageBarriers[barrierIndex];
							barrierDescs.imageBarriersEveryFrame[dstBarrierIndex].image = imageHandle;
						}
					}
					else if (m_boundRenderGraphResources[id].type == BoundResourceType::BUFFER)
					{
						size_t wrapAroundBarriersPerResource = resourceTransitionInfo.wrapAroundBufferBarriers.size();
						VkBuffer bufferHandle = m_boundRenderGraphResources[id].bufferHandles[resIndex]->buffer;
						for (size_t barrierIndex = 0; barrierIndex < wrapAroundBarriersPerResource; ++barrierIndex)
						{
							size_t dstBarrierIndex = preGeneratedBufferBarrierCount * numberOfResourcesBound + resIndex * wrapAroundBarriersPerResource + barrierIndex;
							barrierDescs.bufferBarriersEveryFrame[dstBarrierIndex] = resourceTransitionInfo.wrapAroundBufferBarriers[barrierIndex];
							barrierDescs.bufferBarriersEveryFrame[dstBarrierIndex].buffer = bufferHandle;
						}
					}
					else
					{
						assert(!"unknown binding");
					}

					//memory barriers
					{
						size_t wrapAroundBarriersPerResource = resourceTransitionInfo.wrapAroundMemoryBarriers.size();
						for (size_t barrierIndex = 0; barrierIndex < wrapAroundBarriersPerResource; ++barrierIndex)
						{
							size_t dstBarrierIndex = preGeneratedMemoryBarrierCount * numberOfResourcesBound + resIndex * wrapAroundBarriersPerResource + barrierIndex;
							barrierDescs.memoryBarriersEveryFrame[dstBarrierIndex] = resourceTransitionInfo.wrapAroundMemoryBarriers[barrierIndex];
						}
					}


				}

				//barriers to transition to graph
				for (size_t resIndex = 0; resIndex < numberOfResourcesBound; ++resIndex)
				{

					if (m_boundRenderGraphResources[id].type == BoundResourceType::TEXTURE)
					{
						bool subresourcesShareState = m_boundRenderGraphResources[id].textureHandles[resIndex]->currentLayouts.allSubResourcesShareState();
						const RenderGraphResourceUsage& resourceUsage = m_resourceRequirements.getRenderGraphResourceUsage(nodeIndex, slotIndex);
						TextureHandleVk* imageHandle = m_boundRenderGraphResources[id].textureHandles[resIndex];

						uint32_t mipOffset = 0;
						uint32_t mipCount = 1;
						uint32_t arraySliceOffset = 0;
						uint32_t arraySliceCount = 1;

						if (!subresourcesShareState)
						{
							arraySliceOffset = resourceUsage.arraySliceOffset;
							mipOffset = resourceUsage.mipOffset;
						}

						for (uint32_t arraySlice = arraySliceOffset; arraySlice < arraySliceOffset + arraySliceCount; ++arraySlice)
						{
							for (uint32_t mip = mipOffset; mip < mipOffset + mipCount; ++mip)
							{
								VkImageMemoryBarrier imgBarrier;
								uint32_t fromQueueFamilyIndex = imageHandle->owningQueueFamily;
								TextureHandleVk* imageHandle = m_boundRenderGraphResources[id].textureHandles[resIndex];
								VkImageLayout previousLayout = imageHandle->currentLayouts.getStateForSubResource((size_t)calculateSubresourceIndex(mip, arraySlice, resourceUsage.resourceDescription.mipCount, resourceUsage.resourceDescription.arraySliceCount));
								VkImageLayout currentLayout = yaptUsageToVkImageLayout(resourceUsage.resourceDescription.resourceUsage, resourceUsage.resourceDescription.accessFlags);
								//TODO: correct queue transitions
								if (previousLayout != currentLayout || fromQueueFamilyIndex != toQueueFamilyIndex)
								{
									imgBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
									imgBarrier.pNext = NULL;
									imgBarrier.srcQueueFamilyIndex = fromQueueFamilyIndex;
									imgBarrier.dstQueueFamilyIndex = toQueueFamilyIndex;

									imgBarrier.oldLayout = previousLayout;
									imgBarrier.newLayout = currentLayout;
									imgBarrier.srcAccessMask = VK_ACCESS_NONE;
									imgBarrier.dstAccessMask = yaptUsageAccessToVkAccess(resourceUsage.resourceDescription.resourceUsage, resourceUsage.resourceDescription.accessFlags);

									imgBarrier.subresourceRange.aspectMask = yaptUsageToAspectFlags(resourceUsage.resourceDescription.resourceUsage);
									imgBarrier.subresourceRange.baseArrayLayer = arraySlice;
									imgBarrier.subresourceRange.baseMipLevel = mip;
									imgBarrier.subresourceRange.layerCount = subresourcesShareState ? resourceUsage.resourceDescription.arraySliceCount : 1;
									imgBarrier.subresourceRange.levelCount = subresourcesShareState ? resourceUsage.resourceDescription.mipCount : 1;
									imgBarrier.image = imageHandle->image;
									barrierDescs.imageBarriersOnce.push_back(imgBarrier);
									m_queueTransitionHelper.addFromBarriers(&imgBarrier, 1, true);
								}

							}
						}
					}
					else if (m_boundRenderGraphResources[id].type == BoundResourceType::BUFFER)
					{

						const RenderGraphResourceUsage& resourceUsage = m_resourceRequirements.getRenderGraphResourceUsage(nodeIndex, slotIndex);
						BufferHandleVk* bufferHandle = m_boundRenderGraphResources[id].bufferHandles[resIndex];

						if (bufferHandle->owningQueueFamily != toQueueFamilyIndex)
						{
							VkBufferMemoryBarrier barrier{ VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER, nullptr };
							barrier.srcAccessMask = 0;
							barrier.dstAccessMask = 0;
							barrier.dstQueueFamilyIndex = toQueueFamilyIndex;
							barrier.srcQueueFamilyIndex = bufferHandle->owningQueueFamily;
							barrier.buffer = bufferHandle->buffer;
							barrier.offset = 0;
							barrier.size = VK_WHOLE_SIZE;

							barrierDescs.bufferBarriersOnce.push_back(barrier);
							m_queueTransitionHelper.addFromBarriers(&barrier, 1, true);
						}

					}
					else
					{
						assert(!"unknown binding");
					}
				}
			}
		}
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
		

		if (getVkAccessMaskTransition(usageFrom, accessFlagsFrom, usageTo, accessFlagsTo, fromAccess, toAccess))
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
				bufferBarrierOut.size = VK_WHOLE_SIZE;
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
		else if (needsStorageHazardBarrier)
		{

			memoryBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
			memoryBarrier.pNext = NULL;
			memoryBarrier.srcAccessMask = yaptUsageAccessToVkAccess(usageFrom, accessFlagsFrom);
			memoryBarrier.dstAccessMask = yaptUsageAccessToVkAccess(usageTo, accessFlagsTo);
			mask |= GENERATED_BARRIER_TYPE_MEMORY;
		}
		return mask;
		
	}
	
	
}