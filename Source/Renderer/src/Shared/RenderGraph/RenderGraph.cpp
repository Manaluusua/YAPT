#include <Renderer/Shared/RenderGraph/RenderGraph.h>
#include <Renderer/RendererCommonTypesUtility.h>

#include <algorithm>
#include <unordered_map>
#include <bitset>

#if defined(DEBUG) || defined(_DEBUG) 
	#define ENABLE_GRAPH_SANITY_CHECKS
#endif

namespace YAPT
{
	RenderGraph::RenderGraph(GfxApiHandle h)
		:m_gfxHandle(h),
		m_cmdBufferPool(YAPT_NULL_HANDLE),
		m_numberOfCmdBuffersPerFrame(0)
	{

	}

	RenderGraph::~RenderGraph()
	{
		m_resourceRequirements.reset();

		for (size_t i = 0; i < m_edges.size(); ++i)
		{
			delete m_edges[i];
		}
		m_edges.clear();

		for (size_t i = 0; i < m_nodes.size(); ++i)
		{
			delete m_nodes[i];
		}
		m_nodes.clear();

		Gfx::destroyCommandBufferPool(m_gfxHandle, m_cmdBufferPool);
	}


	RenderNode* RenderGraph::createRenderNode(size_t numberOfConnectionSlots, const RenderGraphNodeSlotDefinition* slotDefinitions, RenderGraphNodeExecutionCallback callback, void* usrData, const char* name)
	{
#ifdef ENABLE_GRAPH_SANITY_CHECKS
		assert(areRenderGraphNodeDefinitionsSane(numberOfConnectionSlots, slotDefinitions));
#endif
		RenderNode* node = createRenderNodeInternal(name, numberOfConnectionSlots, slotDefinitions);
		node->setCallback(callback, usrData);
		m_nodes.push_back(node);
		return node;
	}
	ComputeNode* RenderGraph::createComputeNode(size_t numberOfConnectionSlots, const RenderGraphNodeSlotDefinition* slotDefinitions, RenderGraphNodeExecutionCallback callback, void* usrData, const char* name)
	{
#ifdef ENABLE_GRAPH_SANITY_CHECKS
		assert(!containsRenderTargets(numberOfConnectionSlots, slotDefinitions));
		assert(areRenderGraphNodeDefinitionsSane(numberOfConnectionSlots, slotDefinitions));
#endif
		ComputeNode* node = createComputeNodeInternal(name, numberOfConnectionSlots, slotDefinitions);
		node->setCallback(callback, usrData);
		m_nodes.push_back(node);
		return node;
	}
	RaytraceNode* RenderGraph::createRayTraceNode(size_t numberOfConnectionSlots, const RenderGraphNodeSlotDefinition* slotDefinitions, RenderGraphNodeExecutionCallback callback, void* usrData, const char* name)
	{
#ifdef ENABLE_GRAPH_SANITY_CHECKS
		assert(!containsRenderTargets(numberOfConnectionSlots, slotDefinitions));
		assert(areRenderGraphNodeDefinitionsSane(numberOfConnectionSlots, slotDefinitions));
#endif
		RaytraceNode* node = createRayTraceNodeInternal(name, numberOfConnectionSlots, slotDefinitions);
		node->setCallback(callback, usrData);
		m_nodes.push_back(node);
		return node;
	}

	GenericExecuteNode* RenderGraph::createGenericExecuteNode(size_t numberOfConnectionSlots, const RenderGraphNodeSlotDefinition* slotDefinitions, RenderGraphNodeExecutionCallback callback, void* usrData, const char* name)
	{
#ifdef ENABLE_GRAPH_SANITY_CHECKS
		assert(!containsRenderTargets(numberOfConnectionSlots, slotDefinitions));
		assert(areRenderGraphNodeDefinitionsSane(numberOfConnectionSlots, slotDefinitions));
#endif
		GenericExecuteNode* node = new GenericExecuteNode(name, this, numberOfConnectionSlots, slotDefinitions);
		node->setCallback(callback, usrData);
		registerCustomNode(node);
		return node;
	}

	SwapChainNode* RenderGraph::createSwapChainNode(const char* name)
	{
		SwapChainNode* sn = new SwapChainNode(name, this);
		registerCustomNode(sn);
		return sn;
	}

	void RenderGraph::registerCustomNode(CustomNode* node)
	{
		m_nodes.push_back(node);
		m_customNodes.push_back(node);
	}

	bool RenderGraph::containsRenderTargets(size_t numberOfConnectionSlots, const RenderGraphNodeSlotDefinition* slotDefinitions)
	{
		for (size_t i = 0; i < numberOfConnectionSlots; ++i)
		{
			if ((slotDefinitions[i].resourceDescription.resourceUsage & RESOURCE_USAGE_RENDER_TARGET_TEXTURE) != 0 || (slotDefinitions[i].resourceDescription.resourceUsage & RESOURCE_USAGE_DEPTH_STENCIL_TEXTURE))
			{
				return true;
			}
		}

		return false;
	}

	bool RenderGraph::createEdge(RenderGraphNode* fromNode, size_t fromSlot, RenderGraphNode* toNode, size_t toSlot, uint32_t toArrayOffset, uint32_t toMipOffset)
	{
#ifdef ENABLE_GRAPH_SANITY_CHECKS
		if (!isEdgeValid(fromNode, fromSlot, toNode, toSlot, toArrayOffset, toMipOffset))
		{
			assert(false && "Invalid edge detected");
			return false;
		}
#endif
		RenderGraphNodeEdge* edge = new RenderGraphNodeEdge;
		edge->fromNode = fromNode;
		edge->fromSlot = fromSlot;
		edge->toNode = toNode;
		edge->toSlot = toSlot;
		edge->toArrayOffset = toArrayOffset;
		edge->toMipOffset = toMipOffset;

		fromNode->addEdge(edge);
		toNode->addEdge(edge);

		m_edges.push_back(edge);

		return true;
	}

	bool RenderGraph::isEdgeValid(RenderGraphNode* fromNode, size_t fromSlot, RenderGraphNode* toNode, size_t toSlot, size_t toArrayOffset, size_t toMipOffset)
	{

		if (fromNode == toNode)
		{
			return false;
		}

		if (fromNode->getNumberOfSlots() <= fromSlot)
		{
			return false;
		}

		if (toNode->getNumberOfSlots() <= toSlot)
		{
			return false;
		}



		return true;
	}

	bool RenderGraph::areRenderGraphNodeDefinitionsSane(size_t numberOfConnectionSlots, const RenderGraphNodeSlotDefinition* slotDefinitions)
	{
		for (size_t i = 0; i < numberOfConnectionSlots; ++i)
		{
			std::bitset<sizeof(uint32_t)*8> bits(slotDefinitions->resourceDescription.resourceUsage);
			if (bits.count() > 1 && slotDefinitions->resourceDescription.resourceUsage != RESOURCE_USAGE_DEPTH_STENCIL_TEXTURE)
			{
				YAPT_LOG_FATAL_ERROR("Tried to give a resource more than one usage in node slot %s, %d", __FILE__, __LINE__);
				return false;
			}

			if (slotDefinitions->clearFrequency != RenderNodeClearFrequency::NONE)
			{
				bool clearAllowed = false;
				ResourceUsage allowedClearUsages = RESOURCE_USAGE_RENDER_TARGET_TEXTURE | RESOURCE_USAGE_STENCIL_TEXTURE | RESOURCE_USAGE_DEPTH_TEXTURE;
				
				if ((slotDefinitions->resourceDescription.resourceUsage & allowedClearUsages) == 0)
				{
					YAPT_LOG_FATAL_ERROR("Tried to clear a resource with usage not compatible with clearing (not rendertarget or depth stencil) in node slot %s, %d", __FILE__, __LINE__);
					return false;
				}

			}

		}


		return true;
	}


	RenderGraphResourceId RenderGraph::getRenderGraphResourceIdUsedInSlot(size_t nodeIndex, size_t slot)
	{
		return m_resourceRequirements.getRenderGraphResourceIndex(nodeIndex, slot);
	}

	const RenderGraphResourceDescription& RenderGraph::getRenderGraphResourceDescription(RenderGraphResourceId id) const
	{
		return m_resourceRequirements.getRenderGraphResourceDescription(id);
	}

	void RenderGraph::setRenderGraphResourceBuffers(RenderGraphResourceId id, BufferHandle* handles, size_t handleCount)
	{
		if (getRenderGraphResourceDescription(id).resourceDimensions != ResourceDimension::BUFFER)
		{
			YAPT_LOG_FATAL_ERROR("Tried to set buffer resource to a texture slot! %s, %d", __FILE__, __LINE__)
		}
		m_boundRenderGraphResources[id].type = BoundResourceType::BUFFER;
		m_boundRenderGraphResources[id].bufferHandles.assign(handles, handles + handleCount);
		m_boundRenderGraphResources[id].boundInThisFrame = true;

		resourcesBoundToPipeline(id, handleCount);

		//generate resource views for the bound resource
		const NodeSlotIdentifier* nodeSlotIdentifiers;
		size_t numberOfNodeSlotIdentifiers;
		m_resourceRequirements.getNodeSlotsUsingResource(id, nodeSlotIdentifiers, numberOfNodeSlotIdentifiers);
		for (size_t i = 0; i < numberOfNodeSlotIdentifiers; ++i)
		{
			RenderGraphResourceView& view = m_resourceDataPerNodeSlot[nodeSlotIdentifiers[i].sortedNodeIndex].resourceViewPerSlot[nodeSlotIdentifiers[i].slotIndex];
			view.type = BoundResourceType::BUFFER;
			view.bufferHandles.assign(handles, handles + handleCount);

		}

	}

	bool RenderGraph::isResourceBoundThisFrame(RenderGraphResourceId id) const
	{
		return m_boundRenderGraphResources[id].boundInThisFrame;
	}
	 
	void RenderGraph::setRenderGraphResourceTextures(RenderGraphResourceId id, TextureHandle* handles, size_t handleCount)
	{
		if (getRenderGraphResourceDescription(id).resourceDimensions == ResourceDimension::BUFFER)
		{
			YAPT_LOG_FATAL_ERROR("Tried to set texture resource to a buffer slot! %s, %d", __FILE__, __LINE__)
		}

		m_boundRenderGraphResources[id].type = BoundResourceType::TEXTURE;
		m_boundRenderGraphResources[id].textureHandles.assign(handles, handles + handleCount);;
		m_boundRenderGraphResources[id].boundInThisFrame = true;

		resourcesBoundToPipeline(id, handleCount);

		//generate resource views for the bound resource
		const RenderGraphResourceDescription&  resDesc = m_resourceRequirements.getRenderGraphResourceDescription(id);
		const NodeSlotIdentifier* nodeSlotIdentifiers;
		size_t numberOfNodeSlotIdentifiers;
		m_resourceRequirements.getNodeSlotsUsingResource(id, nodeSlotIdentifiers, numberOfNodeSlotIdentifiers);
		for (size_t i = 0; i < numberOfNodeSlotIdentifiers; ++i)
		{
			TextureViewDesc texViewDesc;
			const RenderGraphResourceUsage& usage = m_resourceRequirements.getRenderGraphResourceUsage(nodeSlotIdentifiers[i].sortedNodeIndex, nodeSlotIdentifiers[i].slotIndex);
			createTextureViewDesc(resDesc, usage, texViewDesc);

			RenderGraphResourceView& view = m_resourceDataPerNodeSlot[nodeSlotIdentifiers[i].sortedNodeIndex].resourceViewPerSlot[nodeSlotIdentifiers[i].slotIndex];
			view.type = BoundResourceType::TEXTURE;

			view.textureViews.clear();
			view.textureViews.reserve(handleCount);

			for (size_t k = 0; k < handleCount; ++k)
			{
				view.textureViews.push_back(Gfx::getTextureView(m_gfxHandle, handles[k], texViewDesc));
			}

			

		}
	}

	BufferHandle RenderGraph::getBufferFromNodeSlot(size_t nodeIndex, size_t slot)
	{
		assert(nodeIndex < m_resourceDataPerNodeSlot.size());
		assert(slot < m_resourceDataPerNodeSlot[nodeIndex].resourceViewPerSlot.size());
		RenderGraphResourceView& view = m_resourceDataPerNodeSlot[nodeIndex].resourceViewPerSlot[slot];
		if (view.type != BoundResourceType::BUFFER)
		{
			YAPT_LOG_FATAL_ERROR("Tried to get buffer resource from a texture slot! %s, %d", __FILE__, __LINE__);
			return YAPT_NULL_HANDLE;
		}
		return view.bufferHandles.size() > 0  ? view.bufferHandles[0] : YAPT_NULL_HANDLE;

	}
	TextureViewHandle RenderGraph::getTextureViewFromNodeSlot(size_t nodeIndex, size_t slot)
	{
		assert(nodeIndex < m_resourceDataPerNodeSlot.size());
		assert(slot < m_resourceDataPerNodeSlot[nodeIndex].resourceViewPerSlot.size());
		RenderGraphResourceView& view = m_resourceDataPerNodeSlot[nodeIndex].resourceViewPerSlot[slot];
		if (view.type != BoundResourceType::TEXTURE)
		{
			YAPT_LOG_FATAL_ERROR("Tried to get texture resource from a buffer slot! %s, %d", __FILE__, __LINE__);
			return YAPT_NULL_HANDLE;
		}
		return view.textureViews.size() > 0 ? view.textureViews[0] : YAPT_NULL_HANDLE;
	}


	void RenderGraph::createTextureViewDesc(const RenderGraphResourceDescription& resourceDesc, const RenderGraphResourceUsage& usage, TextureViewDesc& textureViewDescOut)
	{
		textureViewDescOut.format = usage.resourceDescription.resourceFormat;
		textureViewDescOut.dimensions = usage.resourceDescription.resourceDimensions;
		textureViewDescOut.mipOffset = usage.mipOffset;
		textureViewDescOut.mipCount = usage.resourceDescription.mipCount;
		textureViewDescOut.arraySliceOffset = usage.arraySliceOffset;
		textureViewDescOut.arraySliceCount = usage.resourceDescription.arraySliceCount;
		textureViewDescOut.resourceUsage = usage.resourceDescription.resourceUsage;
	}

	bool RenderGraph::isUsingFullResource(const RenderGraphResourceDescription& resourceDesc, const RenderGraphResourceUsage& to) const
	{

		bool isFullResource = to.mipOffset == 0
			&& to.arraySliceOffset == 0
			&& resourceDesc.arraySliceCount == to.resourceDescription.arraySliceCount
			&& resourceDesc.mipCount == to.resourceDescription.mipCount;


		return isFullResource;
	}

	const ResourceStateDescription& RenderGraph::getLastStateForResource(RenderGraphResourceId resourceId)
	{
		return m_lastStateInGraph[resourceId];
	}

	void RenderGraph::compile()
	{
		sortNodes(m_nodes);

		for (size_t i = 0; i < m_nodes.size(); ++i)
		{
			m_nodes[i]->setSortedIndex(i);
		}
		
		m_resourceRequirements.resolve(m_nodes.data(), m_nodes.size());
		m_boundRenderGraphResources.resize(m_resourceRequirements.getNumberOfRenderGraphResourceDescriptions());

		m_lastStateInGraph.resize(m_resourceRequirements.getNumberOfRenderGraphResourceDescriptions());
		for (size_t resourceId = 0; resourceId < m_lastStateInGraph.size(); ++resourceId)
		{

			const NodeSlotIdentifier* nodeSlotIdentifiers;
			size_t numberOfNodeSlotIdentifiers;
			m_resourceRequirements.getNodeSlotsUsingResource(resourceId, nodeSlotIdentifiers, numberOfNodeSlotIdentifiers);

			const NodeSlotIdentifier& nodeSlot = nodeSlotIdentifiers[numberOfNodeSlotIdentifiers - 1];

			const RenderGraphResourceUsage& usage = m_resourceRequirements.getRenderGraphResourceUsage(nodeSlot.sortedNodeIndex, nodeSlot.slotIndex);

			m_lastStateInGraph[resourceId].accessFlags = usage.resourceDescription.accessFlags;
			m_lastStateInGraph[resourceId].resourceUsage = usage.resourceDescription.resourceUsage;
			m_lastStateInGraph[resourceId].shaderStagesUsedIn = usage.resourceDescription.shaderStages;
		}

		m_resourceDataPerNodeSlot.resize(m_nodes.size());
		for (size_t i = 0; i < m_resourceDataPerNodeSlot.size(); ++i)
		{
			m_resourceDataPerNodeSlot[i].resourceViewPerSlot.resize(m_nodes[i]->getNumberOfSlots());
		}

		//clears
		m_clearsPerNode.resize(m_nodes.size());
		for(size_t i = 0; i < m_nodes.size(); ++i)
		{
			RenderGraphNode* node = m_nodes[i];
			for (size_t slotIndex = 0; slotIndex < node->getNumberOfSlots(); ++slotIndex)
			{
				const RenderGraphNodeSlotDefinition& def = node->getNodeSlotResourceDefinition(slotIndex);
				if (def.clearFrequency == RenderNodeClearFrequency::ALWAYS)
				{
					m_clearsPerNode[i].slotsToClear.push_back(slotIndex);
				}
			}
			
		}

		resolveGraphDependenciesInternal();
	}


	void RenderGraph::createNodeSchedule()
	{
		size_t nodeCount = getNodeCount();
		size_t groupCount = m_numberOfCmdBuffersPerFrame;

		assert(nodeCount > 0);

		size_t nodesPerGroup = nodeCount / groupCount;
		size_t extraNodes = nodeCount % groupCount;

		size_t groupsPerCmdBuffer = groupCount / m_numberOfCmdBuffersPerFrame;
		size_t extraGroups = groupCount % m_numberOfCmdBuffersPerFrame;

		size_t cmdBuffersToUse = std::min(m_numberOfCmdBuffersPerFrame, groupCount);


		m_scheduledRenderGraphNodeGroups.resize(cmdBuffersToUse);

		size_t currentNodeIndex = 0;

		for (size_t bufferIndex = 0; bufferIndex < cmdBuffersToUse; ++bufferIndex)
		{

			ScheduledRenderNodesPerBuffer& perBufferInfo = m_scheduledRenderGraphNodeGroups[bufferIndex];

			size_t currentGroupCount = groupsPerCmdBuffer;
			if (extraGroups > 0)
			{
				currentGroupCount += 1;
				--extraGroups;
			}

			perBufferInfo.renderNodeSequence.resize(currentGroupCount);

			for (size_t groupIndex = 0; groupIndex < currentGroupCount; ++groupIndex)
			{
				size_t currentNodeCount = nodesPerGroup;
				if (extraNodes > 0)
				{
					++currentNodeCount;
					--extraNodes;
				}

				RenderNodeSequence& seq = perBufferInfo.renderNodeSequence[groupIndex];
				seq.offset = currentNodeIndex;
				seq.count = currentNodeCount;
				currentNodeIndex += currentNodeCount;
			}

		}
	}

	void RenderGraph::beginExecution()
	{
		for (CustomNode* cNode : m_customNodes)
		{
			cNode->renderGraphExecutionBegin();
		}
	}

	void RenderGraph::setupScheduling(size_t numberOfCommandBuffers)
	{
		if (m_cmdBufferPool != YAPT_NULL_HANDLE)
		{
			Gfx::destroyCommandBufferPool(m_gfxHandle, m_cmdBufferPool);
		}

		size_t queueIndex = Gfx::getQueueId(getGfxApiHandle(), QueueType::QUEUE_TYPE_GRAPHICS);

		size_t nodeCount = getNodeCount();
		m_numberOfCmdBuffersPerFrame = numberOfCommandBuffers > nodeCount ? nodeCount : numberOfCommandBuffers;

		m_cmdBufferPool = Gfx::createCommandBufferPool(m_gfxHandle, numberOfCommandBuffers, queueIndex, nullptr);
		m_numberOfCmdBuffersPerFrame = numberOfCommandBuffers;

		createNodeSchedule();

	}

	void RenderGraph::execute()
	{
		m_commandBuffersRecording.resize(m_numberOfCmdBuffersPerFrame);


		RenderGraphNode** nodes = getNodes();
		size_t nodeCount = getNodeCount();

		beginExecution();

		//execute nodes (TODO: multithreaded)

		for (size_t cmdBufInd = 0; cmdBufInd < m_numberOfCmdBuffersPerFrame; ++cmdBufInd)
		{
			CommandBufferHandle buff = Gfx::startRecording(m_gfxHandle, m_cmdBufferPool, cmdBufInd);
			m_commandBuffersRecording[cmdBufInd] = buff;

			RenderGraphNodeExecutionContext context;
			context.cmdBuffer = buff;

			for (size_t nodeSequenceIndex = 0; nodeSequenceIndex < m_scheduledRenderGraphNodeGroups[cmdBufInd].renderNodeSequence.size(); ++nodeSequenceIndex)
			{
				const RenderNodeSequence& sequence = m_scheduledRenderGraphNodeGroups[cmdBufInd].renderNodeSequence[nodeSequenceIndex];
				executeNodesInternal(nodes + sequence.offset, sequence.count, context);

			}

			Gfx::stopRecording(m_gfxHandle, buff);
		}

		endExecution();

		Gfx::submitCommandBuffers(m_gfxHandle, m_commandBuffersRecording.data(), m_commandBuffersRecording.size());

		afterRenderGraphSubmit();

		m_commandBuffersRecording.clear();

	}

	void RenderGraph::endExecution()
	{
		for (CustomNode* cNode : m_customNodes)
		{
			cNode->renderGraphExecutionEnd();
		}

		for (size_t i = 0; i < m_boundRenderGraphResources.size(); ++i)
		{
			m_boundRenderGraphResources[i].boundInThisFrame = false;
		}
	}

	void RenderGraph::afterRenderGraphSubmit()
	{
		for (CustomNode* cNode : m_customNodes)
		{
			cNode->renderGraphAfterSubmit();
		}
	}

	void RenderGraph::sortNodes(std::vector<RenderGraphNode*>& nodesToSort)
	{
		//sort nodes (For no just use simple kahns algorithm without parallelization)
		std::vector<RenderGraphNode*> roots;
		roots.reserve(nodesToSort.size());

		std::vector<RenderGraphNode*> sortedNodes;
		sortedNodes.reserve(nodesToSort.size());

		std::unordered_map<RenderGraphNode*, size_t> numberOfInputConnectionsPerNode;
		numberOfInputConnectionsPerNode.reserve(nodesToSort.size());

		//find roots
		for (size_t i = 0; i < nodesToSort.size(); ++i)
		{
			RenderGraphNode* node = nodesToSort[i];

			size_t numberOfInputConnections = 0;

			for (size_t slotIndex = 0; slotIndex < node->getNumberOfSlots(); ++slotIndex)
			{
				numberOfInputConnections += node->getNumberOfInputEdges(slotIndex);
			}

			numberOfInputConnectionsPerNode[node] = numberOfInputConnections;

			if (numberOfInputConnections == 0)
			{
				roots.push_back(node);
			}
		}


		while (roots.size() > 0)
		{
			RenderGraphNode* node = roots.back();
			roots.pop_back();
			
			sortedNodes.push_back(node);

			for (size_t slotIndex = 0; slotIndex < node->getNumberOfSlots(); ++slotIndex)
			{
				for (size_t outputIndex = 0; outputIndex< node->getNumberOfOutputEdges(slotIndex); ++outputIndex)
				{
					const RenderGraphNodeEdge* edge = node->getOutputEdge(slotIndex, outputIndex);
					
					size_t& connectionsCount = numberOfInputConnectionsPerNode[edge->toNode];
					assert(connectionsCount > 0);
					--connectionsCount;
					if (connectionsCount == 0)
					{
						roots.push_back(edge->toNode);
					}
				}

			}

		}

#ifdef ENABLE_GRAPH_SANITY_CHECKS
		for (auto iter = numberOfInputConnectionsPerNode.begin(); iter != numberOfInputConnectionsPerNode.end(); ++iter)
		{
			assert(iter->second == 0);
		}
#endif


		assert(sortedNodes.size() == nodesToSort.size());
		nodesToSort.swap(sortedNodes);
	}

	


}