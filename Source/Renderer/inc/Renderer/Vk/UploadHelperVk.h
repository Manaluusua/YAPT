#pragma once


#include <Common/GrowingMultiProducerPendingList.h>
#include <Renderer/Vk/UploadHeapVk.h>
#include <Renderer/Vk/SubmissionThreadVk.h>
#include <Renderer/Vk/SyncUtilities.h>
#include <Renderer/Vk/CommandBufferPoolVk.h>
namespace YAPT
{

	class UploadHelperVk
	{
		
	public:

		UploadHelperVk(ResourceManagerVk& resourceMngr, SubmissionThreadVk& submitThread, size_t heapSize, size_t numberOfPartitions);
		~UploadHelperVk();

		void* mapCopyRangeForBufferData(VkBuffer buffer, size_t offsetInBytes, size_t sizeInBytes);
		void uploadDataForBuffer(VkBuffer buffer, size_t offsetInBytes, size_t sizeInBytes, const void* data);

		void uploadDataForTexture(VkImage texture, const VkImageCreateInfo& resourceDesc, VkImageLayout currentLayout, VkImageLayout afterCopyLayout,
			size_t arraySliceOffset, size_t arraySliceCount, size_t mipOffset, size_t mipCount, const TextureDataDefinition* textureDataDefinitions);

		void prepareNextUploadBatch();
		bool flushUploadBatch(VkSemaphore* semaphoresToWait, size_t semaphoresToWaitCount, VkSemaphore& signaledSemaphore);



	private:

		struct PartitionSyncData
		{
			VkFence fence;
			bool hasBeenIssued;
		};

		struct TextureUpload
		{
			std::vector<VkBufferImageCopy> copyDescs;
			VkBuffer srcBuffer;
			VkImage dstImage;
			VkImageLayout currentLayout;
			VkImageLayout copyLayout;
			VkImageLayout afterCopyLayout;
		};

		struct BufferUpload
		{
			VkBufferCopy copyDesc;
			VkBuffer srcBuffer;
			VkBuffer dstBuffer;
		};

		bool getHeapMemory(size_t sizeRequested, UploadHeapVk::UploadHeapAllocationInfo& info);
		void getHeapMemoryFromTemporaryHeap(size_t sizeRequested, UploadHeapVk::UploadHeapAllocationInfo& info);
		

		ResourceManagerVk& m_resMngr;
		SubmissionThreadVk& m_submissionThread;
		
		UploadHeapVk m_uploadHeap;
		GrowingMultiProducerPendingList<BufferUpload> m_pendingBufferUploads;
		GrowingMultiProducerPendingList<TextureUpload> m_pendingTextureUploads;
		GrowingMultiProducerPendingList<VkDeviceMemory> m_pendingUnmaps;

		CommandBufferPoolVk m_commandBuffersPool;
		RingSyncUtility m_syncUtility;

	};

}