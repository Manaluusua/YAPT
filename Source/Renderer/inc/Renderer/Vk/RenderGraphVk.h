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
		
		enum GeneratedBarrierTypeBit
		{
			GENERATED_BARRIER_TYPE_NONE = 0,
			GENERATED_BARRIER_TYPE_IMAGE = YAPTBIT(0),
			GENERATED_BARRIER_TYPE_BUFFER = YAPTBIT(1),
			GENERATED_BARRIER_TYPE_MEMORY = YAPTBIT(2)
		};
		typedef uint8_t GeneratedBarrierTypeMask;

		struct AccessFlagsAndLayout
		{
			bool operator==(const AccessFlagsAndLayout& o)
			{
				return (access == o.access) && (layout == o.layout);
			}

			bool operator!=(const AccessFlagsAndLayout& o)
			{
				return !(*this == o);
			}
			VkAccessFlags access;
			VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED;
		};

		struct ResourceSlotBarrierDescription
		{
			VkPipelineStageFlags srcStages;
			VkPipelineStageFlags dstStages;

			std::vector<VkMemoryBarrier> memoryBarriers;
			std::vector<VkBufferMemoryBarrier> bufferBarriers;
			std::vector<VkImageMemoryBarrier> imageBarriers;

			std::vector<VkMemoryBarrier> memoryBarriersForFrame;
			std::vector<VkBufferMemoryBarrier> bufferBarriersForFrame;
			std::vector<VkImageMemoryBarrier> imageBarriersForFrame;

			bool hasUavBarrier;
			bool isFirstUsageForResource;
		};
		
		struct GeneralPerResourceTransitionInformation
		{
			bool useOverriddenBeforeState; //only valid if is first usage for resource in graph, used once to override the "wrap around" barrier for the resource.
			AccessFlagsAndLayout lastStateInGraph;
			AccessFlagsAndLayout overriddenBeforeState; //only valid if is first usage for resource in graph
			std::vector<VkMemoryBarrier> wrapAroundMemoryBarriers;
			std::vector<VkBufferMemoryBarrier> wrapAroundBufferBarriers;
			std::vector<VkImageMemoryBarrier> wrapAroundImageBarriers;
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
		void fillAttachmentDescription(RenderNode* node, size_t slot, VkImageLayout initialLayout, VkImageLayout finalLayout, VkAttachmentDescription& descOut);

		void generateBarriersAndRenderPasses();

		static bool isUsingFullResource(const RenderGraphResourceDescription& resourceDesc, const RenderGraphResourceUsage& to);
		static void calcUsedSubresourceIndices(const RenderGraphResourceDescription& resourceDesc, const RenderGraphResourceUsage& usage, size_t* indicesOut);

		static bool getVkAccessMaskTransition(ResourceUsage usageFrom, AccessFlags accessFlagsFrom, ResourceUsage usageTo, AccessFlags accessFlagsTo, VkAccessFlags& from, VkAccessFlags& to);
		static bool getVkImageLayoutTransition(ResourceUsage usageFrom, AccessFlags accessFlagsFrom, ResourceUsage usageTo, AccessFlags accessFlagsTo, VkImageLayout& from, VkImageLayout& to);
		static VkPipelineStageFlags getPipelineStageFlags(ResourceUsage resourceUsage, AccessFlags accessFlags, ShaderStages shaderStages);

		static GeneratedBarrierTypeMask createBarrierIfRequired(ResourceDimension resDimension, ResourceUsage usageFrom, AccessFlags accessFlagsFrom, ResourceUsage usageTo, AccessFlags accessFlagsTo,
			uint32_t arrayOffset, uint32_t mipOffset, uint32_t arrayCount, uint32_t mipCount, uint32_t srcQueueFamilyIndex, uint32_t dstQueueFamilyIndex,
			VkImageMemoryBarrier& imageBarrierOut, VkBufferMemoryBarrier& bufferBarrierOut, VkMemoryBarrier& memoryBarrier);

		static void fillImageBarrier(const AccessFlagsAndLayout& from, const AccessFlagsAndLayout& to, VkImageMemoryBarrier& barrierOut);
		static void fillBufferBarrier(const AccessFlagsAndLayout& from, const AccessFlagsAndLayout& to, VkBufferMemoryBarrier& barrierOut);
		static void fillMemoryBarrier(const AccessFlagsAndLayout& from, const AccessFlagsAndLayout& to, VkMemoryBarrier& barrierOut);

		std::vector<BarriersPerNode> m_barriers;
		std::vector<GeneralPerResourceTransitionInformation> m_perResourceBarrierInfo;

		std::vector<VkRenderPass> m_renderPasses;

		std::vector<uint32_t> m_queueFamilyIndexPerNode;


	};
	
}