#include <Renderer/Shared/RenderGraph/RenderGraph.h>
#include <assert.h>
namespace YAPT
{
	RenderGraphNode::RenderGraphNode(Type type, const char* name, RenderGraph* graph, size_t numberOfConnectionSlots, const RenderGraphNodeSlotDefinition* slotDefinitions  )
		:m_type(type),
		m_sortedIndex(INVALID_SORTED_INDEX),
		m_graph(graph),
		m_callback(nullptr),
		m_name(name ? name : "")
	{
		m_slotDefinitions.assign(slotDefinitions, slotDefinitions + numberOfConnectionSlots);
		m_slotConnections.resize(numberOfConnectionSlots);

	}


	RenderGraphNode::~RenderGraphNode()
	{

	}

	size_t RenderGraphNode::getNumberOfInputEdges(size_t slot) const
	{
		assert(slot < getNumberOfSlots());
		return m_slotConnections[slot].fromEdges.size();
	}
	const RenderGraphNodeEdge* RenderGraphNode::getInputEdge(size_t slot, size_t inputEdgeIndex) const
	{
		assert(inputEdgeIndex < getNumberOfInputEdges(slot));
		return m_slotConnections[slot].fromEdges[inputEdgeIndex];
	}
	size_t RenderGraphNode::getNumberOfOutputEdges(size_t slot) const
	{
		assert(slot < getNumberOfSlots());
		return m_slotConnections[slot].toEdges.size();
	}
	const RenderGraphNodeEdge* RenderGraphNode::getOutputEdge(size_t slot, size_t outputEdgeIndex) const
	{
		assert(outputEdgeIndex < getNumberOfOutputEdges(slot));
		return m_slotConnections[slot].toEdges[outputEdgeIndex];
	}

	const RenderGraphNodeSlotDefinition& RenderGraphNode::getNodeSlotResourceDefinition(size_t slot) const
	{
		assert(slot < getNumberOfSlots());
		return m_slotDefinitions[slot];
	}

	void RenderGraphNode::addEdge(const RenderGraphNodeEdge* edge)
	{
		if (edge->fromNode == this)
		{
			m_slotConnections[edge->fromSlot].toEdges.push_back(edge);
		}
		else if (edge->toNode == this)
		{
			m_slotConnections[edge->toSlot].fromEdges.push_back(edge);
		}
		else
		{
			assert(false && "not relevant edge for this node");
		}
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