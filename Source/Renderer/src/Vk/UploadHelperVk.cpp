#include <Renderer/Vk/UploadHelperVk.h>
#include <Renderer/Vk/ResourceManagerVk.h>
#include <Renderer/Vk/ResourceHandlesVk.h>

#define INITIAL_MAX_COPY_ENTRIES_PER_COPY_TYPE 1024  


namespace YAPT
{
	UploadHelperVk::UploadHelperVk(ResourceManagerVk& resourceMngr, SubmissionThreadVk& submitThread, size_t heapSize, size_t numberOfPartitions)
		:m_resMngr(resourceMngr),
		m_submissionThread(submitThread),
		m_uploadHeap(resourceMngr, heapSize, numberOfPartitions),
		m_pendingBufferUploads(INITIAL_MAX_COPY_ENTRIES_PER_COPY_TYPE),
		m_pendingTextureUploads(INITIAL_MAX_COPY_ENTRIES_PER_COPY_TYPE),
		m_pendingUnmaps(128),
		m_commandBuffersPool(resourceMngr.getDevice())

	{

		m_syncUtility.initialize(resourceMngr.getDevice(), numberOfPartitions, true, true);
		m_commandBuffersPool.initialize(numberOfPartitions, 1, resourceMngr.getCopyQueue().queueFamilyIndex);
		m_submissionIDs.resize(numberOfPartitions);
	}


	UploadHelperVk::~UploadHelperVk()
	{
		m_syncUtility.deinitialize();
		m_commandBuffersPool.deinitialize();
		
	}

	void* UploadHelperVk::mapCopyRangeForBufferData(VkBuffer buffer, size_t offsetInBytes, size_t sizeInBytes)
	{

		BufferUpload* info = m_pendingBufferUploads.add(1);

		UploadHeapVk::UploadHeapAllocationInfo uploadInfo;
		bool success = getHeapMemory(sizeInBytes, uploadInfo);

		assert(success);
		if (!success)
		{
			return nullptr;
		}

		info->copyDesc.srcOffset = uploadInfo.offsetToHeap;
		info->copyDesc.dstOffset = offsetInBytes;
		info->copyDesc.size = sizeInBytes;

		info->srcBuffer = uploadInfo.uploadBuffer;
		info->dstBuffer = buffer;

		return uploadInfo.mappedPtr;
	}
	void UploadHelperVk::uploadDataForBuffer(VkBuffer buffer, size_t offsetInBytes, size_t sizeInBytes, const void* data)
	{
		void* ptr = mapCopyRangeForBufferData(buffer, offsetInBytes, sizeInBytes);
		assert(ptr != nullptr);
		if (ptr)
		{
			memcpy(ptr, data, sizeInBytes);
		}
		
	}
	
	void UploadHelperVk::uploadDataForTexture(TextureHandleVk* image, const VkImageCreateInfo& resourceDesc, size_t arraySliceOffset, size_t arraySliceCount, size_t mipOffset, size_t mipCount, const TextureDataDefinition* textureDataDefinitions)
	{
		//calculate required memory
		size_t memoryRequiredInBytes = 0;
		{
			uint32_t width = resourceDesc.extent.width;
			uint32_t height = resourceDesc.extent.height;
			uint32_t depth = resourceDesc.extent.depth;

			width = max(1u, width >> uint32_t(mipOffset));
			height = max(1u, height >> uint32_t(mipOffset));
			depth = max(1u, depth >> uint32_t(mipOffset));



			for (size_t mipLevel = 0; mipLevel < mipCount; ++mipLevel)
			{
				size_t mipSize = textureDataDefinitions[mipLevel].rowPitchInBytes * height * depth;
				memoryRequiredInBytes += mipSize;

				width = max(1u, width >> 1);
				height = max(1u, height >> 1);
				depth = max(1u, depth >> 1);
			}

		}
		memoryRequiredInBytes *= arraySliceCount;

		UploadHeapVk::UploadHeapAllocationInfo uploadInfo;
		bool success = getHeapMemory(memoryRequiredInBytes, uploadInfo);

		assert(success);
		if (!success)
		{
			return;
		}
		

		//upload
		char* uploadBufferPtr = uploadInfo.mappedPtr;
		TextureUpload* info = m_pendingTextureUploads.add(1);
		info->dstImage = image;
		info->srcBuffer = uploadInfo.uploadBuffer;
		info->copyDescs.resize(arraySliceCount * mipCount);
		size_t currentUploadBufferOffset = uploadInfo.offsetToHeap;

		for (size_t arraySlice = 0; arraySlice < arraySliceCount; ++arraySlice)
		{
			uint32_t width = resourceDesc.extent.width;
			uint32_t height = resourceDesc.extent.height;
			uint32_t depth = resourceDesc.extent.depth;

			width = max(1u, width >> uint32_t(mipOffset));
			height = max(1u, height >> uint32_t(mipOffset));
			depth = max(1u, depth >> uint32_t(mipOffset));

			for (size_t mipLevel = 0; mipLevel < mipCount; ++mipLevel)
			{
				VkBufferImageCopy& imageCopyStruct = info->copyDescs[arraySlice * mipCount + mipLevel];
				const TextureDataDefinition& texData = textureDataDefinitions[arraySlice * mipCount + mipLevel];

				imageCopyStruct.bufferRowLength = width;//(uint32_t)texData.rowPitchInBytes;
				imageCopyStruct.bufferImageHeight = height;//(uint32_t)texData.rowPitchInBytes * height;
				imageCopyStruct.bufferOffset = currentUploadBufferOffset;
				imageCopyStruct.imageOffset = { 0, 0 ,0 };
				imageCopyStruct.imageExtent = { width, height, depth };
				imageCopyStruct.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				imageCopyStruct.imageSubresource.layerCount = 1;
				imageCopyStruct.imageSubresource.baseArrayLayer = (uint32_t)(arraySlice + arraySliceOffset);
				imageCopyStruct.imageSubresource.mipLevel = (uint32_t)(mipLevel + mipOffset);

				width = max(1u, width >> 1);
				height = max(1u, height >> 1);
				depth = max(1u, depth >> 1);
			}
		}

	}


	
	bool UploadHelperVk::getHeapMemory(size_t sizeRequested, UploadHeapVk::UploadHeapAllocationInfo& info)
	{
		bool success = m_uploadHeap.allocate(sizeRequested, info);
		if (!success)
		{
			getHeapMemoryFromTemporaryHeap(sizeRequested, info);
		}
		return true;
	}

	
	void UploadHelperVk::getHeapMemoryFromTemporaryHeap(size_t sizeRequested, UploadHeapVk::UploadHeapAllocationInfo& info)
	{
		VkBuffer buff;
		void* ptr;
		ResourceManagerVk::AllocatedMemoryInfo memInfo;

		VkBufferCreateInfo buffCreateInfo{};
		buffCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		buffCreateInfo.size = sizeRequested;
		buffCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		buffCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		VkResult res = vkCreateBuffer(m_resMngr.getDevice(), &buffCreateInfo, VK_ALLOC_CB, &buff);
		bool success = m_resMngr.allocateDeviceMemory(0xFFFFFFFF, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, sizeRequested, memInfo);

		res = vkBindBufferMemory(m_resMngr.getDevice(), buff, memInfo.memory, 0);

		checkVkResult(res);
		assert(success);

		vkMapMemory(m_resMngr.getDevice(), memInfo.memory, 0, sizeRequested, 0, &ptr);

		info.mappedPtr = (char*)ptr;
		info.offsetToHeap = 0;
		info.uploadBuffer = buff;

		m_resMngr.deferredDestroyVkResource(buff);
		m_resMngr.deferredDestroyVkResource(memInfo.memory);
		*m_pendingUnmaps.add(1) = memInfo.memory;
	}

	void UploadHelperVk::prepareNextUploadBatch()
	{

		m_syncUtility.nextFrame();
		m_uploadHeap.nextPartition();
	}

	
	bool UploadHelperVk::flushUploadBatch(VkSemaphore* semaphoresToWait, size_t semaphoresToWaitCount, VkSemaphore& signaledSemaphore)
	{

		VkDeviceMemory* pendingUnmaps = m_pendingUnmaps.getAll();
		for (size_t i = 0; i < m_pendingUnmaps.count(); ++i)
		{
			vkUnmapMemory(m_resMngr.getDevice(), pendingUnmaps[i]);
		}

		if (m_pendingBufferUploads.count() == 0 && m_pendingTextureUploads.count() == 0) return false;

		m_commandBuffersPool.clearPool(m_syncUtility.getFrameIndex());

		VkCommandBuffer cmdBuff = m_commandBuffersPool.beginCommandBufferRecording(m_syncUtility.getFrameIndex(), 0);
		

		TextureUpload* textureUploadsList = m_pendingTextureUploads.getAll();
		BufferUpload* bufferUploadsList = m_pendingBufferUploads.getAll();


		// Buffer copies
		{
			std::vector<VkBufferMemoryBarrier> bufferBarriers;
			auto addBufferBarrier = [&bufferBarriers](VkBuffer buffer, uint32_t offset, uint32_t size, uint32_t srcQueueFamily, uint32_t dstQueueFamily)
			{
				VkBufferMemoryBarrier barrier{ VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER, nullptr };
				barrier.srcAccessMask = 0;
				barrier.dstAccessMask = 0;
				barrier.dstQueueFamilyIndex = dstQueueFamily;
				barrier.srcQueueFamilyIndex = srcQueueFamily;
				barrier.buffer = buffer;
				barrier.offset = offset;
				barrier.size = size;


				bufferBarriers.push_back(barrier);
			};

			//pre barriers
			for (size_t i = 0; i < m_pendingBufferUploads.count(); ++i)
			{
				//TODO: actually check if queue family transfer is needed. for now ignored

			}

			if (bufferBarriers.size() > 0)
			{
				vkCmdPipelineBarrier(cmdBuff, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, (uint32_t)bufferBarriers.size(), bufferBarriers.data(), 0, nullptr);
				bufferBarriers.clear();
			}

			//copy
			for (size_t i = 0; i < m_pendingBufferUploads.count(); ++i)
			{
				const BufferUpload& info = bufferUploadsList[i];
				vkCmdCopyBuffer(cmdBuff, info.srcBuffer, info.dstBuffer, 1, &info.copyDesc);
			}

			//post barriers
			for (size_t i = 0; i < m_pendingBufferUploads.count(); ++i)
			{

			}

			if (bufferBarriers.size() > 0)
			{
				vkCmdPipelineBarrier(cmdBuff, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, (uint32_t)bufferBarriers.size(), bufferBarriers.data(), 0, nullptr);
				bufferBarriers.clear();
			}
		}
		
		//image copies
		{
			std::vector<VkImageMemoryBarrier> imageBarriers;
			
			auto addImageBarrier = [&imageBarriers](VkImageLayout oldLayout, VkImageLayout newLayout, VkImage image, uint32_t slice, uint32_t sliceCount, uint32_t mip, uint32_t mipCount, uint32_t srcQueueFamily, uint32_t dstQueueFamily) -> void
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

				imageBarriers.push_back(barrier);
			};

			

			VkImageLayout copyLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			VkImageLayout afterCopyLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			//before copy barriers (image layout transitions)
			for (size_t i = 0; i < m_pendingTextureUploads.count(); ++i)
			{
				const TextureUpload& info = textureUploadsList[i];
				ResourceStateTracker<VkImageLayout>& layouts = info.dstImage->currentLayouts;
				
				for (uint32_t copyDescIndex = 0; copyDescIndex < info.copyDescs.size(); ++copyDescIndex)
				{
					const VkBufferImageCopy& copyDesc = info.copyDescs[copyDescIndex];
					for (uint32_t arrayOffset = 0; arrayOffset < copyDesc.imageSubresource.layerCount; ++arrayOffset)
					{
						uint32_t arraySlice = copyDesc.imageSubresource.baseArrayLayer + arrayOffset;
						uint32_t subresourceIndex = calculateSubresourceIndex(copyDesc.imageSubresource.mipLevel, arraySlice, info.dstImage->createInfo.mipLevels, info.dstImage->createInfo.arrayLayers);
						VkImageLayout currentLayout = layouts.getStateForSubResource(subresourceIndex);

						if (currentLayout != copyLayout)
						{
							addImageBarrier(currentLayout, copyLayout, info.dstImage->image, arraySlice, 1, copyDesc.imageSubresource.mipLevel, 1, info.dstImage->owningQueueFamily, m_resMngr.getCopyQueue().queueFamilyIndex);
							layouts.setStateForSubResource(subresourceIndex, copyLayout);
						}
					}
					
					
				}
			}

			if (imageBarriers.size() > 0)
			{
				vkCmdPipelineBarrier(cmdBuff, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, (uint32_t)imageBarriers.size(), imageBarriers.data());
				imageBarriers.clear();
			}
			

			//issue texture copies
			for (size_t i = 0; i < m_pendingTextureUploads.count(); ++i)
			{
				const TextureUpload& info = textureUploadsList[i];
				vkCmdCopyBufferToImage(cmdBuff, info.srcBuffer, info.dstImage->image, copyLayout, (uint32_t)info.copyDescs.size(), info.copyDescs.data());
			}

			//after copy barriers (image layout transitions)
			for (size_t i = 0; i < m_pendingTextureUploads.count(); ++i)
			{
				const TextureUpload& info = textureUploadsList[i];
				ResourceStateTracker<VkImageLayout>& layouts = info.dstImage->currentLayouts;

				for (uint32_t copyDescIndex = 0; copyDescIndex < info.copyDescs.size(); ++copyDescIndex)
				{
					const VkBufferImageCopy& copyDesc = info.copyDescs[copyDescIndex];
					for (uint32_t arrayOffset = 0; arrayOffset < copyDesc.imageSubresource.layerCount; ++arrayOffset)
					{
						uint32_t arraySlice = copyDesc.imageSubresource.baseArrayLayer + arrayOffset;
						uint32_t subresourceIndex = calculateSubresourceIndex(copyDesc.imageSubresource.mipLevel, arraySlice, info.dstImage->createInfo.mipLevels, info.dstImage->createInfo.arrayLayers);
						VkImageLayout currentLayout = layouts.getStateForSubResource(subresourceIndex);

						if (currentLayout != afterCopyLayout)
						{
							addImageBarrier(currentLayout, afterCopyLayout, info.dstImage->image, arraySlice, 1, copyDesc.imageSubresource.mipLevel, 1, m_resMngr.getCopyQueue().queueFamilyIndex, info.dstImage->owningQueueFamily);
							layouts.setStateForSubResource(subresourceIndex, afterCopyLayout);
						}
					}
				}
			}

			for (size_t i = 0; i < m_pendingTextureUploads.count(); ++i)
			{
				const TextureUpload& info = textureUploadsList[i];
				info.dstImage->currentLayouts.checkSharedState();
			}

			if (imageBarriers.size() > 0)
			{
				vkCmdPipelineBarrier(cmdBuff, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, (uint32_t)imageBarriers.size(), imageBarriers.data());
				imageBarriers.clear();
			}
		}

		

		m_pendingBufferUploads.clear();
		m_pendingTextureUploads.clear();
		m_pendingUnmaps.clear();
		
		m_commandBuffersPool.endCommandBufferRecording(m_syncUtility.getFrameIndex(), 0);

		VkSemaphore signalSem = m_syncUtility.getSemaphoreForThisFrame();

		SubmissionThreadVk::Submission submission;
		submission.semaphoresToSignalCount = 1;
		submission.semaphoresToSignal = &signalSem;
		signaledSemaphore = signalSem;

		submission.semaphoresToWaitCount = semaphoresToWaitCount;
		submission.semaphoresToWait = semaphoresToWait;

		submission.commandLists = &cmdBuff;
		submission.commandListsCount = 1;

		submission.fenceToSignal = m_syncUtility.getFenceForThisFrame();
		
		m_syncUtility.markThisFrameSyncDataIssued();

		m_submissionIDs[m_syncUtility.getFrameIndex()] = m_submissionThread.submit(SubmissionThreadVk::COMMANDQUEUETYPE_COPY, m_resMngr.getCopyQueue().queueIndex, submission);
		return true;

	}

}