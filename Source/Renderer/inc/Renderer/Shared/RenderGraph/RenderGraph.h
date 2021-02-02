#ifndef YAPT_SHARED_RENDERGRAPH_H
#define YAPT_SHARED_RENDERGRAPH_H
#include <Renderer/Shared/GfxTypes.h>
#include "RenderGraphNode.h"
#include "RaytraceNode.h"
#include "RenderNode.h"
#include "ComputeNode.h"
#include "CustomNode.h"
#include "SwapChainNode.h"
#include "GenericExecuteNode.h"

#include "RenderGraphResourceRequirements.h"
#include <vector>


namespace YAPT
{
	 
	class RenderGraph
	{
	public:
		YAPT_NOCOPY(RenderGraph);

		RenderGraph(GfxApiHandle h);
		virtual ~RenderGraph();

		RenderNode* createRenderNode(size_t numberOfConnectionSlots, const RenderGraphNodeSlotDefinition* slotDefinitions, RenderGraphNodeExecutionCallback callback, void* usrData, const char* name = nullptr);
		ComputeNode* createComputeNode(size_t numberOfConnectionSlots, const RenderGraphNodeSlotDefinition* slotDefinitions, RenderGraphNodeExecutionCallback callback, void* usrData,  const char* name = nullptr);
		RaytraceNode* createRayTraceNode(size_t numberOfConnectionSlots, const RenderGraphNodeSlotDefinition* slotDefinitions, RenderGraphNodeExecutionCallback callback, void* usrData, const char* name = nullptr);
		GenericExecuteNode* createGenericExecuteNode(size_t numberOfConnectionSlots, const RenderGraphNodeSlotDefinition* slotDefinitions, RenderGraphNodeExecutionCallback callback, void* usrData, const char* name = nullptr);

		virtual SwapChainNode* createSwapChainNode(const char* name = nullptr);


		bool createEdge(RenderGraphNode* fromNode, size_t fromSlot, RenderGraphNode* toNode, size_t toSlot, uint32_t toArrayOffset = 0, uint32_t toMipOffset = 0);

		RenderGraphResourceId getRenderGraphResourceIdUsedInSlot(size_t nodeIndex, size_t slot);
		const RenderGraphResourceDescription& getRenderGraphResourceDescription(RenderGraphResourceId id) const;

		void setRenderGraphResourceBuffer(RenderGraphResourceId id, BufferHandle handle, const ResourceStateDescription previousState)
		{
			setRenderGraphResourceBuffers(id, &handle, 1, previousState);
		}
		void setRenderGraphResourceTexture(RenderGraphResourceId id, TextureHandle handle, const ResourceStateDescription previousState)
		{
			setRenderGraphResourceTextures(id, &handle, 1, previousState);
		}

		void setRenderGraphResourceBuffers(RenderGraphResourceId id, BufferHandle* handles, size_t handleCount, const ResourceStateDescription previousState);
		void setRenderGraphResourceTextures(RenderGraphResourceId id, TextureHandle* handles, size_t handleCount, const ResourceStateDescription previousState);

		const ResourceStateDescription& getLastStateForResource(RenderGraphResourceId resourceId);

		BufferHandle getBufferFromNodeSlot(size_t nodeIndex, size_t slot);
		TextureViewHandle getTextureViewFromNodeSlot(size_t nodeIndex, size_t slot);

		void compile();

		size_t getNodeCount() const { return m_nodes.size(); }
		RenderGraphNode** getNodes() { return m_nodes.data(); }

		virtual void beginExecution();
		void executeNodes(RenderGraphNode** nodes, size_t nodeCount, const RenderGraphNodeExecutionContext& context);
		virtual void endExecution();
		virtual void afterRenderGraphSubmit();

		GfxApiHandle getGfxApiHandle() const { return m_gfxHandle; }

		bool isResourceBoundThisFrame(RenderGraphResourceId id) const;

		

	protected:

		enum class BoundResourceType
		{
			UNASSIGNED,
			BUFFER,
			TEXTURE
		};

		struct RenderGraphResourceBindings
		{
			BoundResourceType type = BoundResourceType::UNASSIGNED;

			std::vector<TextureHandle> textureHandles;
			std::vector<BufferHandle> bufferHandles;
			bool boundInThisFrame = true;
		};

		struct RenderGraphResourceView
		{
			BoundResourceType type = BoundResourceType::UNASSIGNED;
			std::vector<TextureViewHandle> textureViews;
			std::vector<BufferHandle> bufferHandles;
		};

		struct RenderGraphResourceDataPerNodeSlot
		{
			std::vector<RenderGraphResourceView> resourceViewPerSlot;
		};

		struct ClearsPerNode
		{
			std::vector<size_t> slotsToClear;
		};

		void registerCustomNode(CustomNode* node);

		//sanity checks
		bool containsRenderTargets(size_t numberOfConnectionSlots, const RenderGraphNodeSlotDefinition* slotDefinitions);
		bool isEdgeValid(RenderGraphNode* fromNode, size_t fromSlot, RenderGraphNode* toNode, size_t toSlot, size_t toArrayOffset, size_t toMipOffset);
		bool areRenderGraphNodeDefinitionsSane(size_t numberOfConnectionSlots, const RenderGraphNodeSlotDefinition* slotDefinitions);

		//implemented in derived
		virtual void resolveGraphDependenciesInternal() = 0;
		virtual void executeNodesInternal(RenderGraphNode** nodes, size_t nodeCount, const RenderGraphNodeExecutionContext& context) = 0;
		virtual RenderNode* createRenderNodeInternal(const char* name, size_t numberOfConnectionSlots, const RenderGraphNodeSlotDefinition* slotDefinitions) = 0;
		virtual ComputeNode* createComputeNodeInternal(const char* name, size_t numberOfConnectionSlots, const RenderGraphNodeSlotDefinition* slotDefinitions) = 0;
		virtual RaytraceNode* createRayTraceNodeInternal(const char* name, size_t numberOfConnectionSlots, const RenderGraphNodeSlotDefinition* slotDefinitions) = 0;


		virtual void resourcesBoundToPipeline(RenderGraphResourceId id, const ResourceStateDescription previousState, size_t numberOfResourcesBound) = 0;

		void createTextureViewDesc(const RenderGraphResourceDescription& resourceDesc, const RenderGraphResourceUsage& usage, TextureViewDesc& textureViewDescOut);

		//misc
		inline void invokeNodeCallback(RenderGraphNode* node, const RenderGraphNodeExecutionContext& execContext)
		{
			node->invokeCallback(execContext);
		}
		void sortNodes(std::vector<RenderGraphNode*>& nodesToSort);
		
		
		GfxApiHandle m_gfxHandle;

		RenderGraphResourceRequirements m_resourceRequirements;

		std::vector<ResourceStateDescription> m_lastStateInGraph;

		std::vector<RenderGraphNode*> m_nodes;
		std::vector<CustomNode*> m_customNodes;
		std::vector<RenderGraphNodeEdge*> m_edges;
		std::vector<RenderGraphResourceBindings> m_boundRenderGraphResources;
		std::vector<RenderGraphResourceDataPerNodeSlot> m_resourceDataPerNodeSlot;
		std::vector<ClearsPerNode> m_clearsPerNode;
	};
}

#endif