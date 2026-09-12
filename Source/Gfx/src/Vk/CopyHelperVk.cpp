#include <Gfx/Vk/CopyHelperVk.h>
#include <Gfx/Vk/ResourceManagerVk.h>
#include <Gfx/Vk/ResourceHandlesVk.h>


namespace YAPT
{
	CopyHelperVk::CopyHelperVk(ResourceManagerVk& resourceMngr, SubmissionThreadVk& submitThread)
		:m_resMngr(resourceMngr),
		m_submissionThread(submitThread),
		m_commandBuffersPool(resourceMngr.getDevice()),
		m_frameIndex(0),
		m_batchIndexInFrame(0),
		m_batchSemaphoresToWait(nullptr),
		m_batchSemaphoresToWaitCount(0),
		m_batchLastSignaledSemaphore(VK_NULL_HANDLE)
	{

	}

	CopyHelperVk::~CopyHelperVk()
	{
		deinitialize();
	}

	void CopyHelperVk::initialize(size_t numberOfPartitions, size_t numberOfBatchesPerFrame)
	{
		assert(numberOfBatchesPerFrame > 0);

		m_commandBuffersPool.initialize(numberOfPartitions, numberOfBatchesPerFrame, m_resMngr.getCopyQueue().queueFamilyIndex);

		m_syncUtilities.resize(numberOfBatchesPerFrame);
		for (size_t i = 0; i < m_syncUtilities.size(); ++i)
		{
			m_syncUtilities[i].initialize(m_resMngr.getDevice(), numberOfPartitions, true, true);
		}

		//a release and an acquire transition per batch
		m_queueTransitionHelper.initialize(m_resMngr.getDevice(), numberOfPartitions, numberOfBatchesPerFrame * 2);

		m_submissionIDs.resize(numberOfPartitions * numberOfBatchesPerFrame);

		m_frameIndex = m_syncUtilities[0].getFrameIndex();
		m_batchIndexInFrame = 0;
	}

	void CopyHelperVk::deinitialize()
	{
		for (size_t i = 0; i < m_syncUtilities.size(); ++i)
		{
			m_syncUtilities[i].deinitialize();
		}
		m_syncUtilities.clear();

		m_commandBuffersPool.deinitialize();
		m_queueTransitionHelper.deinitialize();
	}

	void CopyHelperVk::prepareNextFrame()
	{
		for (size_t i = 0; i < m_syncUtilities.size(); ++i)
		{
			m_syncUtilities[i].nextFrame();
		}

		m_frameIndex = m_syncUtilities[0].getFrameIndex();
		m_batchIndexInFrame = 0;

		//every batch slot of this partition was waited for above, so all of the pool's command buffers are done
		m_commandBuffersPool.resetPool(m_frameIndex);
	}

	void CopyHelperVk::addBufferBarrier(VkBuffer buffer, VkDeviceSize offset, VkDeviceSize size, uint32_t srcQueueFamily, uint32_t dstQueueFamily)
	{
		VkBufferMemoryBarrier barrier{ VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER, nullptr };
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = 0;
		barrier.dstQueueFamilyIndex = dstQueueFamily;
		barrier.srcQueueFamilyIndex = srcQueueFamily;
		barrier.buffer = buffer;
		barrier.offset = offset;
		barrier.size = size;

		m_allBufferBarriers.push_back(barrier);
	}

	void CopyHelperVk::addImageBarrier(VkImageLayout oldLayout, VkImageLayout newLayout, VkImageLayout afterCopyLayout, VkImage image, uint32_t slice, uint32_t sliceCount, uint32_t mip, uint32_t mipCount, uint32_t srcQueueFamily, uint32_t dstQueueFamily)
	{
		VkImageMemoryBarrier barrier{ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER, nullptr };
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = 0;
		barrier.dstQueueFamilyIndex = dstQueueFamily;
		barrier.srcQueueFamilyIndex = srcQueueFamily;
		barrier.image = image;
		barrier.newLayout = newLayout;
		barrier.oldLayout = oldLayout;
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		barrier.subresourceRange.baseArrayLayer = slice;
		barrier.subresourceRange.baseMipLevel = mip;
		barrier.subresourceRange.layerCount = sliceCount;
		barrier.subresourceRange.levelCount = mipCount;

		m_allImageBarriers.push_back(barrier);
		m_afterCopyLayout.push_back(afterCopyLayout);
	}

	void CopyHelperVk::addBarriersForImageCopy(TextureHandleVk* image, uint32_t mip, uint32_t arraySliceOffset, uint32_t arraySliceCount, VkImageLayout copyLayout, VkImageLayout layoutAfterCopy)
	{
		const uint32_t copyQueueFamilyIndex = m_resMngr.getCopyQueue().queueFamilyIndex;
		ResourceStateTracker<VkImageLayout>& layouts = image->currentLayouts;

		for (uint32_t arrayOffset = 0; arrayOffset < arraySliceCount; ++arrayOffset)
		{
			uint32_t arraySlice = arraySliceOffset + arrayOffset;
			uint32_t subresourceIndex = calculateSubresourceIndex(mip, arraySlice, image->createInfo.mipLevels, image->createInfo.arrayLayers);
			VkImageLayout currentLayout = layouts.getStateForSubResource(subresourceIndex);

			uint32_t sliceCount = 1;
			if ((image->createInfo.flags & VK_IMAGE_CREATE_2D_ARRAY_COMPATIBLE_BIT) != 0)
			{
				sliceCount = VK_REMAINING_ARRAY_LAYERS;
			}

			VkImageLayout finalLayout = layoutAfterCopy == RESTORE_LAYOUT_BEFORE_COPY ? currentLayout : layoutAfterCopy;
			if (finalLayout == VK_IMAGE_LAYOUT_UNDEFINED)
			{
				//nothing meaningful to restore (the subresource has never been used), leaving it in the copy layout is
				//the only legal option as a barrier may not transition into UNDEFINED
				finalLayout = copyLayout;
			}

			if (currentLayout != copyLayout || image->owningQueueFamily != copyQueueFamilyIndex)
			{
				addImageBarrier(currentLayout, copyLayout, finalLayout, image->image, arraySlice, sliceCount, mip, 1, image->owningQueueFamily, copyQueueFamilyIndex);
				layouts.setStateForSubResource(subresourceIndex, finalLayout);
			}
		}
	}

	void CopyHelperVk::addBarriersForBufferCopy(VkBuffer buffer, uint32_t owningQueueFamilyIndex, VkDeviceSize offset, VkDeviceSize size)
	{
		const uint32_t copyQueueFamilyIndex = m_resMngr.getCopyQueue().queueFamilyIndex;
		if (owningQueueFamilyIndex == copyQueueFamilyIndex) return;

		addBufferBarrier(buffer, offset, size, owningQueueFamilyIndex, copyQueueFamilyIndex);
	}

	void CopyHelperVk::clearBarriers()
	{
		m_allImageBarriers.clear();
		m_allBufferBarriers.clear();
		m_afterCopyLayout.clear();
	}

	void CopyHelperVk::issuePipelineBarriers(VkCommandBuffer cmdBuff)
	{
		if (m_allBufferBarriers.size() == 0 && m_allImageBarriers.size() == 0) return;

		vkCmdPipelineBarrier(cmdBuff, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, (uint32_t)m_allBufferBarriers.size(), m_allBufferBarriers.data(), (uint32_t)m_allImageBarriers.size(), m_allImageBarriers.data());
	}

	void CopyHelperVk::reverseBarriers()
	{
		for (size_t i = 0; i < m_allBufferBarriers.size(); ++i)
		{
			std::swap(m_allBufferBarriers[i].dstQueueFamilyIndex, m_allBufferBarriers[i].srcQueueFamilyIndex);
		}

		for (size_t i = 0; i < m_allImageBarriers.size(); ++i)
		{
			std::swap(m_allImageBarriers[i].dstQueueFamilyIndex, m_allImageBarriers[i].srcQueueFamilyIndex);
			m_allImageBarriers[i].oldLayout = m_allImageBarriers[i].newLayout;
			m_allImageBarriers[i].newLayout = m_afterCopyLayout[i];
		}
	}

	VkCommandBuffer CopyHelperVk::beginCopyBatch(VkSemaphore* semaphoresToWait, size_t semaphoresToWaitCount)
	{
		assert(m_batchIndexInFrame < m_syncUtilities.size());

		m_batchSemaphoresToWait = semaphoresToWait;
		m_batchSemaphoresToWaitCount = semaphoresToWaitCount;
		m_batchLastSignaledSemaphore = VK_NULL_HANDLE;

		//release the resources from the queue families currently owning them
		m_queueTransitionHelper.clearBarriers();
		m_queueTransitionHelper.addFromBarriers(m_allBufferBarriers.data(), m_allBufferBarriers.size(), true);
		m_queueTransitionHelper.addFromBarriers(m_allImageBarriers.data(), m_allImageBarriers.size(), true);
		m_queueTransitionHelper.issueTransitionBarriers(m_submissionThread, m_batchIndexInFrame * 2, semaphoresToWait, semaphoresToWaitCount, m_batchLastSignaledSemaphore);

		VkCommandBuffer cmdBuff = m_commandBuffersPool.beginCommandBufferRecording(m_frameIndex, m_batchIndexInFrame);

		//acquire them on the copy queue and get them into the layout the copy needs
		issuePipelineBarriers(cmdBuff);

		return cmdBuff;
	}

	VkSemaphore CopyHelperVk::endCopyBatch(VkCommandBuffer cmdBuff, VkFence fenceToSignalAfterCopies)
	{
		//release the resources from the copy queue again and get them into the layout they are expected to be in
		//after the copy
		reverseBarriers();
		issuePipelineBarriers(cmdBuff);

		m_commandBuffersPool.endCommandBufferRecording(m_frameIndex, m_batchIndexInFrame);

		RingSyncUtility& syncUtility = m_syncUtilities[m_batchIndexInFrame];
		VkSemaphore copySemaphore = syncUtility.getSemaphoreForThisFrame();

		SubmissionThreadVk::Submission submission;
		submission.semaphoresToSignalCount = 1;
		submission.semaphoresToSignal = &copySemaphore;

		submission.semaphoresToWaitCount = m_batchLastSignaledSemaphore == VK_NULL_HANDLE ? m_batchSemaphoresToWaitCount : 1;
		submission.semaphoresToWait = m_batchLastSignaledSemaphore == VK_NULL_HANDLE ? m_batchSemaphoresToWait : &m_batchLastSignaledSemaphore;

		submission.commandLists = &cmdBuff;
		submission.commandListsCount = 1;

		submission.fenceToSignal = syncUtility.getFenceForThisFrame();

		syncUtility.markThisFrameSyncDataIssued();

		m_submissionIDs[m_frameIndex * m_syncUtilities.size() + m_batchIndexInFrame] = m_submissionThread.submit(COMMANDQUEUETYPE_COPY, m_resMngr.getCopyQueue().queueIndex, submission);

		if (fenceToSignalAfterCopies != VK_NULL_HANDLE)
		{
			//a fence signal submitted to a queue is ordered after everything submitted to it before, so an empty
			//submission is all it takes to have the caller's fence signaled once the copies above are done
			SubmissionThreadVk::Submission fenceSubmission;
			fenceSubmission.fenceToSignal = fenceToSignalAfterCopies;

			m_submissionThread.submit(COMMANDQUEUETYPE_COPY, m_resMngr.getCopyQueue().queueIndex, fenceSubmission);
		}

		//acquire the resources back on the queue families owning them
		VkSemaphore transitionSemaphore = VK_NULL_HANDLE;
		m_queueTransitionHelper.clearBarriers();
		m_queueTransitionHelper.addFromBarriers(m_allBufferBarriers.data(), m_allBufferBarriers.size(), false);
		m_queueTransitionHelper.addFromBarriers(m_allImageBarriers.data(), m_allImageBarriers.size(), false);
		bool transitionsIssued = m_queueTransitionHelper.issueTransitionBarriers(m_submissionThread, m_batchIndexInFrame * 2 + 1, &copySemaphore, 1, transitionSemaphore);

		++m_batchIndexInFrame;

		//without any transitions nothing consumed the copy submission's semaphore, so that is the one the caller has
		//to wait on
		return transitionsIssued ? transitionSemaphore : copySemaphore;
	}

}
