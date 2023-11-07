#pragma once

#include <Renderer/Shared/RenderGraph/RenderGraph.h>
#include <Renderer/Vk/QueueTransitionHelperVk.h>

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

			std::vector<VkMemoryBarrier> preGeneratedMemoryBarriers;
			std::vector<VkBufferMemoryBarrier> preGeneratedBufferBarriers;
			std::vector<VkImageMemoryBarrier> preGeneratedImageBarriers;

			std::vector<VkMemoryBarrier> memoryBarriersEveryFrame;
			std::vector<VkBufferMemoryBarrier> bufferBarriersEveryFrame;
			std::vector<VkImageMemoryBarrier> imageBarriersEveryFrame;

			std::vector<VkMemoryBarrier> memoryBarriersOnce;
			std::vector<VkBufferMemoryBarrier> bufferBarriersOnce;
			std::vector<VkImageMemoryBarrier> imageBarriersOnce;

			bool hasUavBarrier;
			bool isFirstUsageForResource;
			bool hasValidBarriers;
		};
		
		struct GeneralPerResourceTransitionInformation
		{
			AccessFlagsAndLayout lastStateInGraph;
			std::vector<VkMemoryBarrier> wrapAroundMemoryBarriers;
			std::vector<VkBufferMemoryBarrier> wrapAroundBufferBarriers;
			std::vector<VkImageMemoryBarrier> wrapAroundImageBarriers;
			bool useWrapAroundBarriers; //when executing the graph for the first time, we don't use wrap around barriers but the "once" barriers that transition resource to graph required layout/queue
		};

		struct BarriersPerNode
		{
			std::vector<ResourceSlotBarrierDescription> perSlotDesc;

			std::vector<VkMemoryBarrier> memBarriersCache;
			std::vector<VkBufferMemoryBarrier> bufBarriersCache;
			std::vector<VkImageMemoryBarrier> imgBarriersCache;
			
		};
		
		virtual void resolveGraphDependenciesInternal() override;
		virtual RenderNode* createRenderNodeInternal(const char* name, size_t numberOfConnectionSlots, const RenderGraphNodeSlotDefinition* slotDefinitions) final;
		virtual ComputeNode* createComputeNodeInternal(const char* name, size_t numberOfConnectionSlots, const RenderGraphNodeSlotDefinition* slotDefinitions) final;
		virtual RaytraceNode* createRayTraceNodeInternal(const char* name, size_t numberOfConnectionSlots, const RenderGraphNodeSlotDefinition* slotDefinitions) final;
		virtual void executeNodesInternal(RenderGraphNode** nodes, size_t nodeCount, const RenderGraphNodeExecutionContext& context) final;
		virtual void resourcesBoundToPipeline(RenderGraphResourceId id, size_t numberOfResourcesBound) final;

		virtual void beginExecution() final;
		virtual void endExecution() final;
		virtual void afterRenderGraphSubmit() final;

		void updateBarriersForResource(RenderGraphResourceId id, size_t numberOfResourcesBound);


		void issueBarriers(size_t nodeIndex, CommandBufferHandle buffer);

		void prepareNodeExecution(RenderNodeVk* node, const RenderGraphNodeExecutionContext& context);
		void prepareNodeExecution(ComputeNodeVk* node, const RenderGraphNodeExecutionContext& context);
		void prepareNodeExecution(RaytraceNodeVk* node, const RenderGraphNodeExecutionContext& context);

		void endNodeExecution(RenderNodeVk* node, const RenderGraphNodeExecutionContext& context);
		void endNodeExecution(ComputeNodeVk* node, const RenderGraphNodeExecutionContext& context);
		void endNodeExecution(RaytraceNodeVk* node, const RenderGraphNodeExecutionContext& context);

		VkFramebuffer createFrameBuffer(RenderNodeVk* node) const;
		VkRenderPass createRenderPass(RenderNode* node) const;
		void fillAttachmentDescription(RenderNode* node, size_t slot, VkImageLayout initialLayout, VkImageLayout finalLayout, VkAttachmentDescription& descOut) const;

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
		std::vector<VkFramebuffer> m_frameBuffers;

		std::vector<uint32_t> m_queueFamilyIndexPerNode;
		QueueTransitionHelperVk m_queueTransitionHelper;


	};
	
}