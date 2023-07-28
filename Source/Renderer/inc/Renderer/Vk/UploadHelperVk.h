#pragma once


#include <Common/GrowingMultiProducerPendingList.h>
#include <Renderer/Vk/UploadHeapVk.h>
#include <Renderer/Vk/SubmissionThreadVk.h>
#include <Renderer/Vk/SyncUtilities.h>
#include <Renderer/Vk/CommandBufferPoolVk.h>
#include <Renderer/Vk/QueueTransitionHelperVk.h>
#include <unordered_map>

namespace YAPT
{

	class UploadHelperVk
	{
		
	public:

		UploadHelperVk(ResourceManagerVk& resourceMngr, SubmissionThreadVk& submitThread, size_t heapSize, size_t numberOfPartitions);
		~UploadHelperVk();

		void* mapCopyRangeForBufferData(BufferHandleVk* buffer, size_t offsetInBytes, size_t sizeInBytes);
		void uploadDataForBuffer(BufferHandleVk* buffer, size_t offsetInBytes, size_t sizeInBytes, const void* data);

		void uploadDataForTexture(TextureHandleVk* image, const VkImageCreateInfo& resourceDesc, size_t arraySliceOffset, size_t arraySliceCount, size_t mipOffset, size_t mipCount, const TextureDataDefinition* textureDataDefinitions);

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
			TextureHandleVk* dstImage;
		};

		struct BufferUpload
		{
			VkBufferCopy copyDesc;
			VkBuffer srcBuffer;
			VkBuffer dstBuffer;
			uint32_t dstBufferQueueFamilyIndex;
		};

		struct QueueFamilyTransitionData
		{
			QueueFamilyTransitionData(VkDevice device, uint32_t numberOfPartitions, uint32_t queueFamilyIndex);
			~QueueFamilyTransitionData();

			CommandBufferPoolVk commandBuffersPool;
			RingSyncUtility preSyncUtility;
			RingSyncUtility postSyncUtility;
			std::vector<VkBufferMemoryBarrier> bufferBarriers;
			std::vector<VkImageMemoryBarrier> imageBarriers;
		};


		bool getHeapMemory(size_t sizeRequested, UploadHeapVk::UploadHeapAllocationInfo& info);
		void getHeapMemoryFromTemporaryHeap(size_t sizeRequested, UploadHeapVk::UploadHeapAllocationInfo& info);

		ResourceManagerVk& m_resMngr;
		SubmissionThreadVk& m_submissionThread;
		
		UploadHeapVk m_uploadHeap;
		GrowingMultiProducerPendingList<BufferUpload> m_pendingBufferUploads;
		GrowingMultiProducerPendingList<TextureUpload> m_pendingTextureUploads;
		GrowingMultiProducerPendingList<VkDeviceMemory> m_pendingUnmaps;
		std::vector<SubmissionThreadVk::SubmissionId> m_submissionIDs;
		CommandBufferPoolVk m_commandBuffersPool;
		RingSyncUtility m_syncUtility;
		QueueTransitionHelperVk m_queueTransitionHelper;
		VkSemaphore m_lastSignaledSemaphore;

		
		std::vector<VkBufferMemoryBarrier> m_allBufferBarriers;
		std::vector<VkImageMemoryBarrier> m_allImageBarriers;;

	};

}