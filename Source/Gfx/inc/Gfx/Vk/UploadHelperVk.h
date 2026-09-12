#pragma once


#include <Common/GrowingMultiProducerPendingList.h>
#include <Gfx/Vk/CopyHelperVk.h>
#include <Gfx/Vk/UploadHeapVk.h>

namespace YAPT
{

	class UploadHelperVk : public CopyHelperVk
	{

	public:

		UploadHelperVk(ResourceManagerVk& resourceMngr, SubmissionThreadVk& submitThread, size_t heapSize, size_t numberOfPartitions);
		~UploadHelperVk();

		void* mapCopyRangeForBufferData(VkBuffer buffer, uint32_t owningQueueFamilyIndex, size_t offsetInBytes, size_t sizeInBytes);
		void uploadDataForBuffer(BufferHandleVk* buffer, size_t offsetInBytes, size_t sizeInBytes, const void* data);
		void uploadDataForBuffer(VkBuffer buffer, uint32_t owningQueueFamilyIndex, size_t offsetInBytes, size_t sizeInBytes, const void* data);

		void uploadDataForTexture(TextureHandleVk* image, const VkImageCreateInfo& resourceDesc, size_t arraySliceOffset, size_t arraySliceCount, size_t mipOffset, size_t mipCount, const TextureDataDefinition* textureDataDefinitions, const ResourceStateDescription& afterUploadUsage);

		void prepareNextUploadBatch();
		bool flushUploadBatch(VkSemaphore* semaphoresToWait, size_t semaphoresToWaitCount, VkSemaphore& signaledSemaphore);



	private:

		struct TextureUpload
		{
			std::vector<VkBufferImageCopy> copyDescs;
			VkBuffer srcBuffer;
			TextureHandleVk* dstImage;
			VkImageLayout layoutAfterCopy;
		};

		struct BufferUpload
		{
			VkBufferCopy copyDesc;
			VkBuffer srcBuffer;
			VkBuffer dstBuffer;
			uint32_t dstBufferQueueFamilyIndex;
		};


		bool getHeapMemory(size_t sizeRequested, UploadHeapVk::UploadHeapAllocationInfo& info);
		void getHeapMemoryFromTemporaryHeap(size_t sizeRequested, UploadHeapVk::UploadHeapAllocationInfo& info);

		UploadHeapVk m_uploadHeap;
		GrowingMultiProducerPendingList<BufferUpload> m_pendingBufferUploads;
		GrowingMultiProducerPendingList<TextureUpload> m_pendingTextureUploads;
		GrowingMultiProducerPendingList<Allocation> m_pendingUnmaps;
	};

}
