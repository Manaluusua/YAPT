#pragma once

#include <Renderer/Shared/RenderGraph/RenderGraph.h>


namespace YAPT
{
	class RenderNodeVk;
	class ComputeNodeVk;
	class RaytraceNodeVk;
	class DescriptorHeapVk;
	class RenderGraphVk final : public RenderGraph 
	{
	public:
		RenderGraphVk(GfxApiHandle h);
		virtual ~RenderGraphVk() override;

	private:
		
		struct ResourceSlotBarrierDescription
		{
			/*std::vector<D3D12_RESOURCE_BARRIER> beforeBarriersPerResource;
			std::vector<D3D12_RESOURCE_BARRIER> afterBarriersPerResource;

			std::vector<D3D12_RESOURCE_BARRIER> currentBeforeBarriers;
			std::vector<D3D12_RESOURCE_BARRIER> currentAfterBarriers;

			size_t numberOfTransitionBarriers;
			D3D12_RESOURCE_STATES transitionedToState; //state transitioned to, redundantly stated here
			bool hasUavBarrier;
			bool isFirstUsageForResource;*/
		};
		
		struct GeneralPerResourceTransitionInformation
		{
			/*bool useOverriddenBeforeState; //only valid if is first usage for resource in graph, used once to override the "wrap around" barrier for the resource.
			D3D12_RESOURCE_STATES lastStateInGraph;
			D3D12_RESOURCE_STATES overriddenBeforeState; //only valid if is first usage for resource in graph*/
		};

		struct BarriersPerNode
		{
			std::vector<ResourceSlotBarrierDescription> perSlotDesc;
		};
		
		virtual void resolveGraphDependenciesInternal() override;
		virtual RenderNode* createRenderNodeInternal(const char* name, size_t numberOfConnectionSlots, const RenderGraphNodeSlotDefinition* slotDefinitions) final;
		virtual ComputeNode* createComputeNodeInternal(const char* name, size_t numberOfConnectionSlots, const RenderGraphNodeSlotDefinition* slotDefinitions) final;
		virtual RaytraceNode* createRayTraceNodeInternal(const char* name, size_t numberOfConnectionSlots, const RenderGraphNodeSlotDefinition* slotDefinitions) final;
		virtual void executeNodesInternal(RenderGraphNode** nodes, size_t nodeCount, const RenderGraphNodeExecutionContext& context) final;
		virtual void resourcesBoundToPipeline(RenderGraphResourceId id, const ResourceStateDescription previousState, size_t numberOfResourcesBound) final;

		void updateBarriersForResource(RenderGraphResourceId id, const ResourceStateDescription previousState, size_t numberOfResourcesBound);

		void issuePreBarriers(size_t nodeIndex, CommandBufferHandle buffer);
		void issuePostBarriers(size_t nodeIndex, CommandBufferHandle buffer);

		void prepareNodeExecution(RenderNodeVk* node, const RenderGraphNodeExecutionContext& context);
		void prepareNodeExecution(ComputeNodeVk* node, const RenderGraphNodeExecutionContext& context);
		void prepareNodeExecution(RaytraceNodeVk* node, const RenderGraphNodeExecutionContext& context);

		void handleClears(RenderGraphNode* node, const RenderGraphNodeExecutionContext& context);

		VkRenderPass createRenderPass(RenderNode* node);

		void generateBarriersAndRenderPasses();

		static bool isUsingFullResource(const RenderGraphResourceDescription& resourceDesc, const RenderGraphResourceUsage& to);
		static void calcUsedSubresourceIndices(const RenderGraphResourceDescription& resourceDesc, const RenderGraphResourceUsage& usage, size_t* indicesOut);

		std::vector<BarriersPerNode> m_barriers;
		std::vector<GeneralPerResourceTransitionInformation> m_perResourceBarrierInfo;

		std::vector<VkRenderPass> m_renderPasses;


	};
	
}