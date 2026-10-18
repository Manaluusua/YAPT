#ifndef YAPT_SHARED_RENDERGRAPHNODE_H
#define YAPT_SHARED_RENDERGRAPHNODE_H

#include "RenderGraphCommon.h"
#include <Common/ChunkedArenaAllocator.h>

namespace YAPT
{
	struct RenderGraphNodeExecutionContext
	{
		CommandBufferHandle cmdBuffer;
	};

	enum RenderGraphNodeSlotFlagBits
	{
		RGNS_FLAG_NONE,
		RGNS_FLAG_ALWAYS_REQUIRE_LOAD,
		RGNS_FLAG_ALWAYS_REQUIRE_STORE
	};

	typedef Flags RenderGraphNodeSlotFlags;

	struct RenderGraphNodeSlotDefinition
	{
		RenderGraphResourceDescription resourceDescription;
		RenderNodeClearFrequency clearFrequency;
		ClearValue clearValue;
		RenderGraphNodeSlotFlags flags;
	};

	struct RenderGraphTextureSlotDefinition : public RenderGraphNodeSlotDefinition
	{
		RenderGraphTextureSlotDefinition(ResourceDimension resourceDimensions, ResourceFormat resourceFormat, ResourceUsage resourceUsage,
			AccessFlags accessFlags, ShaderStages shaderStages, uint32_t mipCount, uint32_t arraySliceCount,
			RenderNodeClearFrequency clear = RenderNodeClearFrequency::NONE, ClearValue clearValue = ClearValue(), 
			RenderGraphNodeSlotFlags flags = RGNS_FLAG_NONE)
		{
			resourceDescription.resourceDimensions = resourceDimensions;
			resourceDescription.resourceFormat = resourceFormat;
			resourceDescription.resourceUsage = resourceUsage;
			resourceDescription.accessFlags = accessFlags;
			resourceDescription.shaderStages = shaderStages;
			resourceDescription.mipCount = mipCount;
			resourceDescription.arraySliceCount = arraySliceCount;
			this->clearFrequency = clear;
			this->clearValue = clearValue;
			this->flags = flags;
		}
	};

	struct RenderGraphBufferSlotDefinition : public RenderGraphNodeSlotDefinition
	{
		RenderGraphBufferSlotDefinition(ResourceUsage resourceUsage,
			AccessFlags accessFlags, ShaderStages shaderStages,
			RenderGraphNodeSlotFlags flags = RGNS_FLAG_NONE)
		{
			resourceDescription.resourceDimensions = ResourceDimension::BUFFER;
			resourceDescription.resourceFormat = ResourceFormat::UNKNOWN;
			resourceDescription.resourceUsage = resourceUsage;
			resourceDescription.accessFlags = accessFlags;
			resourceDescription.shaderStages = shaderStages;
			resourceDescription.mipCount = 1;
			resourceDescription.arraySliceCount = 1;
			this->clearFrequency = RenderNodeClearFrequency::NONE;
			this->clearValue = {};
			this->flags = flags;
		}
	};

	typedef void(*RenderGraphNodeExecutionCallback)(RenderGraphNode* node, const RenderGraphNodeExecutionContext& execContext, void* usrData);

	class RenderGraph;

	class RenderGraphNode
	{
		friend class RenderGraph;
		friend class RenderGraphResourceRequirements;
	public:

		const static size_t INVALID_SORTED_INDEX = size_t(-1);

		YAPT_NOCOPY(RenderGraphNode);

		enum class Type
		{
			COMPUTE,
			RENDER,
			RAYTRACE,
			CUSTOM
		};

		size_t getNumberOfSlots() const { return m_numberOfSlots; }
		Type getType() const { return m_type; }

		RenderGraph* getGraph() { return m_graph; }

		const RenderGraphNodeSlotDefinition& getNodeSlotResourceDefinition(size_t slot) const;
		
		size_t getNumberOfInputEdges(size_t slot) const;
		const RenderGraphNodeEdge* getInputEdge(size_t slot, size_t inputEdgeIndex) const;
		size_t getNumberOfOutputEdges(size_t slot) const;
		const RenderGraphNodeEdge* getOutputEdge(size_t slot, size_t outputEdgeIndex) const;

		size_t getSortedIndex() const { return m_sortedIndex; }

		RenderGraphResourceId getRenderGraphResourceIdForSlot(size_t slotIndex);

		const char* getName() const { return m_name; }

	protected:
		struct SlotEdges
		{
			RenderGraphNodeEdge* inHead = nullptr;
			RenderGraphNodeEdge* inTail = nullptr;
			RenderGraphNodeEdge* outHead = nullptr;
			RenderGraphNodeEdge* outTail = nullptr;

			const RenderGraphNodeEdge** in = nullptr;
			const RenderGraphNodeEdge** out = nullptr;

			uint32_t inCount = 0;
			uint32_t outCount = 0;
		};


		RenderGraphNode(Type type, const char* name, RenderGraph* graph, size_t numberOfConnectionSlots, const RenderGraphNodeSlotDefinition* slotDefinitions);
		virtual ~RenderGraphNode();

		void setCallback(RenderGraphNodeExecutionCallback callback, void* usrData);

		void invokeCallback(const RenderGraphNodeExecutionContext& execContext);

		void setSortedIndex(size_t index) { m_sortedIndex = index; }
		void setCreationIndex(size_t index) { m_creationIndex = index; }
		size_t getCreationIndex() const { return m_creationIndex; }
		void setResolvedResourceFormat(size_t slot, ResourceFormat format);
		void addEdge(RenderGraphNodeEdge* edge);
		void flattenEdges();

		ChunkedArenaAllocator& getArena();

		virtual void renderGraphCompilationComplete() {}

		const Type m_type;
		RenderGraph* m_graph;

		RenderGraphNodeSlotDefinition* m_slotDefinitions;
		SlotEdges* m_slotConnections;
		size_t m_numberOfSlots;

		size_t m_sortedIndex;
		size_t m_creationIndex;
		bool m_edgesFlattened;

		RenderGraphNodeExecutionCallback m_callback;
		void* m_usrData;

		const char* m_name;
	};


}

#endif