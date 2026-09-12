#include <Gfx/Vk/UploadHelperVk.h>
#include <Gfx/Vk/ResourceManagerVk.h>
#include <Gfx/Vk/ResourceHandlesVk.h>
#include <Gfx/Vk/YaptToVkConversions.h>

#define INITIAL_MAX_COPY_ENTRIES_PER_COPY_TYPE 1024  


namespace YAPT
{
	UploadHelperVk::UploadHelperVk(ResourceManagerVk& resourceMngr, SubmissionThreadVk& submitThread, size_t heapSize, size_t numberOfPartitions)
		:CopyHelperVk(resourceMngr, submitThread),
		m_uploadHeap(resourceMngr, heapSize, numberOfPartitions),
		m_pendingBufferUploads(INITIAL_MAX_COPY_ENTRIES_PER_COPY_TYPE),
		m_pendingTextureUploads(INITIAL_MAX_COPY_ENTRIES_PER_COPY_TYPE),
		m_pendingUnmaps(128)

	{
		//all the uploads gathered during a frame are flushed as a single batch
		initialize(numberOfPartitions, 1);
	}


	UploadHelperVk::~UploadHelperVk()
	{

	}

	void* UploadHelperVk::mapCopyRangeForBufferData(VkBuffer buffer, uint32_t owningQueueFamilyIndex, size_t offsetInBytes, size_t sizeInBytes)
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
		info->dstBufferQueueFamilyIndex = owningQueueFamilyIndex;
		info->srcBuffer = uploadInfo.uploadBuffer;
		info->dstBuffer = buffer;

		return uploadInfo.mappedPtr;
	}

	void UploadHelperVk::uploadDataForBuffer(VkBuffer buffer, uint32_t owningQueueFamilyIndex, size_t offsetInBytes, size_t sizeInBytes, const void* data)
	{
		void* ptr = mapCopyRangeForBufferData(buffer, owningQueueFamilyIndex,  offsetInBytes, sizeInBytes);
		assert(ptr != nullptr);
		if (ptr)
		{
			memcpy(ptr, data, sizeInBytes);
		}
	}

	void UploadHelperVk::uploadDataForBuffer(BufferHandleVk* buffer, size_t offsetInBytes, size_t sizeInBytes, const void* data)
	{
		uploadDataForBuffer(buffer->buffer, buffer->owningQueueFamily, offsetInBytes, sizeInBytes, data);
	}
	
	void UploadHelperVk::uploadDataForTexture(TextureHandleVk* image, const VkImageCreateInfo& resourceDesc, size_t arraySliceOffset, size_t arraySliceCount, size_t mipOffset, size_t mipCount, const TextureDataDefinition* textureDataDefinitions, const ResourceStateDescription& afterUploadUsage)
	{
		//calculate required memory
		size_t memoryRequiredInBytes = 0;
		size_t texelBlockSize = getTexelBlockSize(resourceDesc.format);
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
				memoryRequiredInBytes += mipSize + texelBlockSize;

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
		info->layoutAfterCopy = yaptUsageToVkImageLayout(afterUploadUsage.resourceUsage, afterUploadUsage.accessFlags);
		size_t currentUploadBufferOffset = align(uploadInfo.offsetToHeap, texelBlockSize);
		size_t mappedBufferOffset = currentUploadBufferOffset - uploadInfo.offsetToHeap;

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

				size_t copySize = texData.rowPitchInBytes * height * depth;
				memcpy(uploadBufferPtr + mappedBufferOffset, texData.data, copySize);

				width = max(1u, width >> 1);
				height = max(1u, height >> 1);
				depth = max(1u, depth >> 1);

				size_t nextBatchOffset = align(copySize, texelBlockSize);

				currentUploadBufferOffset += nextBatchOffset;
				mappedBufferOffset += nextBatchOffset;
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
		Allocation alloc;

		VkBufferCreateInfo buffCreateInfo{};
		buffCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		buffCreateInfo.size = sizeRequested;
		buffCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		buffCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		VmaAllocationCreateInfo allocInfo = {};
		allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
		allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;
		bool success = m_resMngr.createBufferVk(buffCreateInfo, allocInfo, 0, &buff, &alloc);

		assert(success);

		info.mappedPtr = (char*)m_resMngr.map(alloc);
		info.offsetToHeap = 0;
		info.uploadBuffer = buff;

		m_resMngr.deferredDestroyVkResource(buff);
		m_resMngr.deferredFreeAlloc(alloc);
		*m_pendingUnmaps.add(1) = alloc;
	}

	void UploadHelperVk::prepareNextUploadBatch()
	{

		prepareNextFrame();
		m_uploadHeap.nextPartition();
	}




	bool UploadHelperVk::flushUploadBatch(VkSemaphore* semaphoresToWait, size_t semaphoresToWaitCount, VkSemaphore& signaledSemaphore)
	{

		const VkImageLayout copyLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;

		Allocation* pendingUnmaps = m_pendingUnmaps.getAll();
		for (size_t i = 0; i < m_pendingUnmaps.count(); ++i)
		{
			m_resMngr.unmap(pendingUnmaps[i]);
		}

		if (m_pendingBufferUploads.count() == 0 && m_pendingTextureUploads.count() == 0) return false;



		//clear all previous barriers
		clearBarriers();


		TextureUpload* textureUploadsList = m_pendingTextureUploads.getAll();
		BufferUpload* bufferUploadsList = m_pendingBufferUploads.getAll();

		//image barriers
		{

			for (size_t i = 0; i < m_pendingTextureUploads.count(); ++i)
			{
				const TextureUpload& info = textureUploadsList[i];

				for (uint32_t copyDescIndex = 0; copyDescIndex < info.copyDescs.size(); ++copyDescIndex)
				{
					const VkBufferImageCopy& copyDesc = info.copyDescs[copyDescIndex];
					addBarriersForImageCopy(info.dstImage, copyDesc.imageSubresource.mipLevel, copyDesc.imageSubresource.baseArrayLayer, copyDesc.imageSubresource.layerCount, copyLayout, info.layoutAfterCopy);
				}
			}
		}

		//buffer barriers
		{
			for (size_t i = 0; i < m_pendingBufferUploads.count(); ++i)
			{
				const BufferUpload& info = bufferUploadsList[i];
				addBarriersForBufferCopy(info.dstBuffer, info.dstBufferQueueFamilyIndex, info.copyDesc.dstOffset, info.copyDesc.size);
			}

		}

		VkCommandBuffer cmdBuff = beginCopyBatch(semaphoresToWait, semaphoresToWaitCount);

		// Buffer copies
		{

			//copy
			for (size_t i = 0; i < m_pendingBufferUploads.count(); ++i)
			{
				const BufferUpload& info = bufferUploadsList[i];
				vkCmdCopyBuffer(cmdBuff, info.srcBuffer, info.dstBuffer, 1, &info.copyDesc);
			}

		}

		//image copies
		{

			//issue texture copies
			for (size_t i = 0; i < m_pendingTextureUploads.count(); ++i)
			{
				const TextureUpload& info = textureUploadsList[i];
				vkCmdCopyBufferToImage(cmdBuff, info.srcBuffer, info.dstImage->image, copyLayout, (uint32_t)info.copyDescs.size(), info.copyDescs.data());
			}
		}

		m_pendingBufferUploads.clear();
		m_pendingTextureUploads.clear();
		m_pendingUnmaps.clear();

		signaledSemaphore = endCopyBatch(cmdBuff, VK_NULL_HANDLE);

		return true;

	}

}