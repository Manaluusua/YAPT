#include <Gfx/RenderGraph/RenderGraph.h>
#include <assert.h>
namespace YAPT
{
	RenderGraphNode::RenderGraphNode(Type type, const char* name, RenderGraph* graph, size_t numberOfConnectionSlots, const RenderGraphNodeSlotDefinition* slotDefinitions  )
		:m_type(type),
		m_graph(graph),
		m_slotDefinitions(nullptr),
		m_slotConnections(nullptr),
		m_numberOfSlots(numberOfConnectionSlots),
		m_sortedIndex(INVALID_SORTED_INDEX),
		m_creationIndex(INVALID_SORTED_INDEX),
		m_edgesFlattened(false),
		m_callback(nullptr),
		m_usrData(nullptr),
		m_name(nullptr)
	{
		ChunkedArenaAllocator& arena = getArena();
		m_name = arena.copyString(name);
		m_slotDefinitions = arena.copyArray(slotDefinitions, numberOfConnectionSlots);
		m_slotConnections = arena.allocateArray<SlotEdges>(numberOfConnectionSlots);
	}


	RenderGraphNode::~RenderGraphNode()
	{

	}

	ChunkedArenaAllocator& RenderGraphNode::getArena()
	{
		return m_graph->getArena();
	}

	size_t RenderGraphNode::getNumberOfInputEdges(size_t slot) const
	{
		assert(slot < getNumberOfSlots());
		return m_slotConnections[slot].inCount;
	}
	const RenderGraphNodeEdge* RenderGraphNode::getInputEdge(size_t slot, size_t inputEdgeIndex) const
	{
		assert(inputEdgeIndex < getNumberOfInputEdges(slot));
		const SlotEdges& edges = m_slotConnections[slot];
		if (m_edgesFlattened)
		{
			return edges.in[inputEdgeIndex];
		}

		const RenderGraphNodeEdge* edge = edges.inHead;
		for (size_t i = 0; i < inputEdgeIndex; ++i)
		{
			edge = edge->nextInToSlot;
		}
		return edge;
	}
	size_t RenderGraphNode::getNumberOfOutputEdges(size_t slot) const
	{
		assert(slot < getNumberOfSlots());
		return m_slotConnections[slot].outCount;
	}
	const RenderGraphNodeEdge* RenderGraphNode::getOutputEdge(size_t slot, size_t outputEdgeIndex) const
	{
		assert(outputEdgeIndex < getNumberOfOutputEdges(slot));
		const SlotEdges& edges = m_slotConnections[slot];
		if (m_edgesFlattened)
		{
			return edges.out[outputEdgeIndex];
		}

		const RenderGraphNodeEdge* edge = edges.outHead;
		for (size_t i = 0; i < outputEdgeIndex; ++i)
		{
			edge = edge->nextOutFromSlot;
		}
		return edge;
	}

	const RenderGraphNodeSlotDefinition& RenderGraphNode::getNodeSlotResourceDefinition(size_t slot) const
	{
		assert(slot < getNumberOfSlots());
		return m_slotDefinitions[slot];
	}

	void RenderGraphNode::addEdge(RenderGraphNodeEdge* edge)
	{
		assert(!m_edgesFlattened);

		if (edge->fromNode == this)
		{
			SlotEdges& edges = m_slotConnections[edge->fromSlot];
			edge->nextOutFromSlot = nullptr;
			if (edges.outTail)
			{
				edges.outTail->nextOutFromSlot = edge;
			}
			else
			{
				edges.outHead = edge;
			}
			edges.outTail = edge;
			++edges.outCount;
		}
		else if (edge->toNode == this)
		{
			SlotEdges& edges = m_slotConnections[edge->toSlot];
			edge->nextInToSlot = nullptr;
			if (edges.inTail)
			{
				edges.inTail->nextInToSlot = edge;
			}
			else
			{
				edges.inHead = edge;
			}
			edges.inTail = edge;
			++edges.inCount;
		}
		else
		{
			assert(false && "not relevant edge for this node");
		}
	}

	void RenderGraphNode::flattenEdges()
	{
		if (m_edgesFlattened) return;

		ChunkedArenaAllocator& arena = getArena();
		for (size_t slot = 0; slot < m_numberOfSlots; ++slot)
		{
			SlotEdges& edges = m_slotConnections[slot];

			if (edges.inCount > 0)
			{
				edges.in = static_cast<const RenderGraphNodeEdge**>(arena.allocate(sizeof(const RenderGraphNodeEdge*) * edges.inCount, alignof(const RenderGraphNodeEdge*)));
				size_t i = 0;
				for (const RenderGraphNodeEdge* e = edges.inHead; e != nullptr; e = e->nextInToSlot)
				{
					edges.in[i++] = e;
				}
				assert(i == edges.inCount);
			}

			if (edges.outCount > 0)
			{
				edges.out = static_cast<const RenderGraphNodeEdge**>(arena.allocate(sizeof(const RenderGraphNodeEdge*) * edges.outCount, alignof(const RenderGraphNodeEdge*)));
				size_t i = 0;
				for (const RenderGraphNodeEdge* e = edges.outHead; e != nullptr; e = e->nextOutFromSlot)
				{
					edges.out[i++] = e;
				}
				assert(i == edges.outCount);
			}
		}

		m_edgesFlattened = true;
	}


	void RenderGraphNode::setCallback(RenderGraphNodeExecutionCallback callback, void* usrData)
	{
		m_callback = callback;
		m_usrData = usrData;
	}

	void RenderGraphNode::invokeCallback(const RenderGraphNodeExecutionContext& execContext)
	{
		if (m_callback == nullptr) return;
		m_callback(this, execContext, m_usrData);
	}

	void RenderGraphNode::setResolvedResourceFormat(size_t slot, ResourceFormat format)
	{
		assert(slot < getNumberOfSlots());
		assert(m_slotDefinitions[slot].resourceDescription.resourceFormat == ResourceFormat::UNKNOWN || m_slotDefinitions[slot].resourceDescription.resourceFormat == format);
		m_slotDefinitions[slot].resourceDescription.resourceFormat = format;
	}

	RenderGraphResourceId RenderGraphNode::getRenderGraphResourceIdForSlot(size_t slotIndex)
	{
		return m_graph->getRenderGraphResourceIdUsedInSlot(getSortedIndex(), slotIndex);
	}

}
