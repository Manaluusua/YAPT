#pragma once


#include <Gfx/Vk/SubmissionThreadVk.h>
#include <Gfx/Vk/SyncUtilities.h>
#include <Gfx/Vk/CommandBufferPoolVk.h>
#include <Gfx/Vk/QueueTransitionHelperVk.h>
#include <vector>

namespace YAPT
{
	class ResourceManagerVk;
	struct TextureHandleVk;


	class CopyHelperVk
	{

	public:

	protected:

		//layoutAfterCopy value asking for the layout a subresource was in before the copy to be restored afterwards
		static constexpr VkImageLayout RESTORE_LAYOUT_BEFORE_COPY = VK_IMAGE_LAYOUT_UNDEFINED;

		CopyHelperVk(ResourceManagerVk& resourceMngr, SubmissionThreadVk& submitThread);
		~CopyHelperVk();

		void initialize(size_t numberOfPartitions, size_t numberOfBatchesPerFrame);
		void deinitialize();


		void prepareNextFrame();

		void addBarriersForImageCopy(TextureHandleVk* image, uint32_t mip, uint32_t arraySliceOffset, uint32_t arraySliceCount, VkImageLayout copyLayout, VkImageLayout layoutAfterCopy);
		void addBarriersForBufferCopy(VkBuffer buffer, uint32_t owningQueueFamilyIndex, VkDeviceSize offset, VkDeviceSize size);
		void clearBarriers();


		VkCommandBuffer beginCopyBatch(VkSemaphore* semaphoresToWait, size_t semaphoresToWaitCount);


		VkSemaphore endCopyBatch(VkCommandBuffer cmdBuff, VkFence fenceToSignalAfterCopies);

		ResourceManagerVk& m_resMngr;
		SubmissionThreadVk& m_submissionThread;

	private:

		void addBufferBarrier(VkBuffer buffer, VkDeviceSize offset, VkDeviceSize size, uint32_t srcQueueFamily, uint32_t dstQueueFamily);
		void addImageBarrier(VkImageLayout oldLayout, VkImageLayout newLayout, VkImageLayout afterCopyLayout, VkImage image, uint32_t slice, uint32_t sliceCount, uint32_t mip, uint32_t mipCount, uint32_t srcQueueFamily, uint32_t dstQueueFamily);

		void issuePipelineBarriers(VkCommandBuffer cmdBuff);
		void reverseBarriers();

		std::vector<SubmissionThreadVk::SubmissionId> m_submissionIDs;
		CommandBufferPoolVk m_commandBuffersPool;
		std::vector<RingSyncUtility> m_syncUtilities; 
		QueueTransitionHelperVk m_queueTransitionHelper;

		std::vector<VkBufferMemoryBarrier> m_allBufferBarriers;
		std::vector<VkImageMemoryBarrier> m_allImageBarriers;
		std::vector<VkImageLayout> m_afterCopyLayout;

		size_t m_frameIndex;
		size_t m_batchIndexInFrame;


		VkSemaphore* m_batchSemaphoresToWait;
		size_t m_batchSemaphoresToWaitCount;
		VkSemaphore m_batchLastSignaledSemaphore;
	};

}
