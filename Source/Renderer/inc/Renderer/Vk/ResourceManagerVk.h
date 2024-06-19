#ifndef YAPT_VK_RESOURCEMANAGER_H
#define YAPT_VK_RESOURCEMANAGER_H


#include <Renderer/Shared/GfxApi.h>
#include <Common/CommonUtilities.h>
#include <Common/GrowingMultiProducerPendingList.h>

#include <Renderer/Vk/ResourceUtilityVk.h>

#include <vector>
#include <assert.h>

#define DEFAULT_UPLOAD_HEAP_SIZE 128 * 1e6 //128mb


namespace YAPT
{
	class SubmissionThreadVk;
	class UploadHelperVk;
	class CommandBufferPoolVk;
	class AccelerationStructureBuilder;

	class ResourceManagerVk
	{
	public:
		struct AllocatedMemoryInfo
		{
			VkDeviceMemory memory;
			VkMemoryPropertyFlags flags;
		};

		ResourceManagerVk(VkPhysicalDevice physicalDevice, SubmissionThreadVk& submissionThread, const QueueDefinitionVk& copyQueue, VkDevice device, size_t pipelineLength);
		~ResourceManagerVk();

		void prepare();
		void flushPreFrameUploads(VkSemaphore semaphoreToWaitBeforeUploads);
		void flushFrameUploads();

		BufferHandleVk* createBuffer(const YAPT::BufferDesc& desc, const ResourceStateDescription& initialState, uint32_t owningQueueFamily, const char* name);
		TextureHandleVk* createTexture(const YAPT::TextureDesc& desc, const ResourceStateDescription& initialState, uint32_t owningQueueFamily, const char* name);
		void destroyBuffer(BufferHandleVk* handle);
		void destroyTexture(TextureHandleVk* handle);

		VkDevice getDevice() const { return m_device; }

		bool allocateDeviceMemory(uint32_t allowedMemoryTypes, VkMemoryPropertyFlags requiredFlags, size_t size, AllocatedMemoryInfo& out);

		void upload(BufferHandleVk* handle, size_t offsetInBytes, size_t sizeInBytes, const void* data, GpuUploadStage heapType);
		void* map(BufferHandleVk* handle, size_t offsetInBytes, size_t sizeInBytes, GpuUploadStage heapType);
		void unmap(BufferHandleVk* handle);
		void upload(TextureHandleVk* image, size_t arraySliceOffset, size_t arraySliceCount, size_t mipOffset, size_t mipCount, const TextureDataDefinition* textureDataDefinitions, GpuUploadStage heapType);

		ShaderModuleHandle createShaderModule(const char* filepath, ShaderModuleType moduleType, const char* entryPoint, const ShaderModuleDefine* defines, size_t defineCount);
		void destroyShaderModule(ShaderModuleHandle m);

		CommandBufferPoolVk* createAutoResetCommandBufferPool(size_t numberOfBuffersPerFrame, size_t queueId);
		void destroyAutoResetCommandBufferPool(CommandBufferPoolVk* pool);
		void resetCommandBufferPools(size_t pipelineFrameIndex);

		const QueueDefinitionVk& getCopyQueue() const { return m_copyQueue; }
		AccelerationStructureBuilder* getAccelerationStructureBuilder() { return m_accStructBuilder; }

		void overrideLastSignaledSemaphore(VkSemaphore sem) { m_lastSignaledSemaphore = sem; }
		VkSemaphore getLastSignaledSemaphore() { return m_lastSignaledSemaphore; }
		void clearLastSignaledSemaphore() { m_lastSignaledSemaphore = VK_NULL_HANDLE; }


		template<typename T>
		inline void deferredDestroyVkResource(T resource)
		{
			//TODO: implement properly later, remove locks and store the resource to something better than just guessing the safe size
			DestroyResourceEntry entry;

			assert(sizeof(T) <= sizeof(char[32]));

			memcpy(entry.data, &resource, sizeof(T));
			entry.cb = [](VkDevice device, char data[32], const VkAllocationCallbacks* alloc)
			{
				T t;
				memcpy(&t, data, sizeof(T));

				destroyVkDeviceChild(device, t, alloc);
			};

			std::unique_lock<std::mutex> lock(m_destroyObjectsLock);
			m_pendingDestroyedObjects[m_destroyObjectsIndex].push_back(entry);
		}

	private:

		typedef void (*DestroyVkResourceCallback)(VkDevice device, char data[32], const VkAllocationCallbacks* alloc);

		struct DestroyResourceEntry
		{
			DestroyVkResourceCallback cb;
			char data[32];
		};

		void copyViaUploadHeap(BufferHandleVk* buffer, size_t offsetInBytes, size_t sizeInBytes, const void* data, GpuUploadStage heapType);
		void copyViaUploadHeap(TextureHandleVk* image, size_t arraySliceOffset, size_t arraySliceCount,
			size_t mipOffset, size_t mipCount, const TextureDataDefinition* textureDataDefinitions, GpuUploadStage heapType);
		void* mapCopyRangeFromUploadHeap(BufferHandleVk* buffer, size_t offsetInBytes, size_t sizeInBytes, GpuUploadStage heapType);

		
		
		void deinitialize();	
		
		SubmissionThreadVk& m_submissionThread;
		const QueueDefinitionVk& m_copyQueue;
		size_t m_pipelineLength;

		VkSemaphore m_lastSignaledSemaphore;
		
		VkDevice m_device;
		VkPhysicalDevice m_physicalDevice;
		VkPhysicalDeviceMemoryProperties m_memoryProps;

		UploadHelperVk* m_preFrameUploads;
		UploadHelperVk* m_duringFrameUploads;

		AccelerationStructureBuilder* m_accStructBuilder;

		//command buffer pools
		std::vector<CommandBufferPoolVk*> m_commandBufferPools;
		std::mutex m_commandBufferPoolsMutex;

		//pending destruction lists
		std::vector<std::vector<DestroyResourceEntry>> m_pendingDestroyedObjects;
		std::mutex m_destroyObjectsLock;
		size_t m_destroyObjectsIndex;

	};
}
#endif