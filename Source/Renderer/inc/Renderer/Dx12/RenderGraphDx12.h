#pragma once

#include <Renderer/Shared/RenderGraph/RenderGraph.h>
#include <Renderer/Dx12/DescriptorHeapAllocatorDx12.h>

namespace YAPT
{
	class RenderNodeDx12;
	class ComputeNodeDx12;
	class RaytraceNodeDx12;
	class DescriptorHeapDx12;
	class RenderGraphDx12 final : public RenderGraph 
	{
	public:
		RenderGraphDx12(GfxApiHandle h);
		virtual ~RenderGraphDx12() override;

	private:
		
		struct ResourceSlotBarrierDescription
		{
			//pregenerated barriers for a resource that is bound to this slot (these are generated before any knowledge of the exact resource and the number of them)
			std::vector<D3D12_RESOURCE_BARRIER> preGeneratedBeforeBarriersPerResource;
			std::vector<D3D12_RESOURCE_BARRIER> preGeneratedAfterBarriersPerResource;

			//actual barriers generated for the resource in slot. Note that there might be more than 1 resource in this slot, so the number of these barriers can be more than the pregenerated one.
			std::vector<D3D12_RESOURCE_BARRIER> currentBeforeBarriers;
			std::vector<D3D12_RESOURCE_BARRIER> currentAfterBarriers;
			std::vector<D3D12_RESOURCE_BARRIER> overriddenBeforeBarriers;

			size_t numberOfTransitionBarriers;
			D3D12_RESOURCE_STATES transitionedToState; //state transitioned to, redundantly stated here
			bool hasUavBarrier;
			bool isFirstUsageForResource;
		};
		
		struct GeneralPerResourceTransitionInformation
		{
			bool useOverriddenBeforeState; //only valid if is first usage for resource in graph, used once to override the "wrap around" barrier for the resource.
			D3D12_RESOURCE_STATES lastStateInGraph;
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
		virtual void resourcesBoundToPipeline(RenderGraphResourceId id, size_t numberOfResourcesBound) final;

		void updateBarriersForResource(RenderGraphResourceId id, size_t numberOfResourcesBound);

		void issuePreBarriers(size_t nodeIndex, CommandBufferHandle buffer);
		void issuePostBarriers(size_t nodeIndex, CommandBufferHandle buffer);

		void prepareNodeExecution(RenderNodeDx12* node, const RenderGraphNodeExecutionContext& context);
		void prepareNodeExecution(ComputeNodeDx12* node, const RenderGraphNodeExecutionContext& context);
		void prepareNodeExecution(RaytraceNodeDx12* node, const RenderGraphNodeExecutionContext& context);

		void handleClears(RenderGraphNode* node, const RenderGraphNodeExecutionContext& context);

		void createRenderTargetResources();

		void generateBarriers();
		void mergeReadOnlyBarriers();
		void generateSplitBarriers();
		D3D12_RESOURCE_STATES getLastStateInGraphInternal(size_t resourceId);

		static D3D12_RESOURCE_STATES getD3D12StateFromResourceUsage(const RenderGraphResourceUsage& usage);
		static D3D12_RESOURCE_STATES getD3D12StateFromResourceUsage(const ResourceStateDescription& previousState);
		static bool isUsingFullResource(const RenderGraphResourceDescription& resourceDesc, const RenderGraphResourceUsage& to);
		static void calcUsedSubresourceIndices(const RenderGraphResourceDescription& resourceDesc, const RenderGraphResourceUsage& usage, size_t* indicesOut);

		const bool useSplitBarriers = false;

		std::vector<BarriersPerNode> m_barriers;
		std::vector<GeneralPerResourceTransitionInformation> m_perResourceBarrierInfo;

		DescriptorHeapDx12* m_rtvHeap;
		DescriptorHeapDx12* m_dsvHeap;
	};
	
}