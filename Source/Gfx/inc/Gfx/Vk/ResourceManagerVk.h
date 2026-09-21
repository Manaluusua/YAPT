#ifndef YAPT_VK_RESOURCEMANAGER_H
#define YAPT_VK_RESOURCEMANAGER_H


#include <Gfx/GfxApi.h>
#include <Common/CommonUtilities.h>
#include <Common/GrowingMultiProducerPendingList.h>
#include <Gfx/Vk/VkExtensions.h>
#include <Gfx/Vk/ResourceUtilityVk.h>
#include <vector>
#include <assert.h>
#include <Gfx/Common/SyncPrimitiveManager.h>

#define DEFAULT_UPLOAD_HEAP_SIZE 128 * 1e6 //128mb
#define DEFAULT_MAX_DOWNLOADS_PER_FRAME 4

namespace YAPT
{
	class SubmissionThreadVk;
	class UploadHelperVk;
	class DownloadHelperVk;
	class CommandBufferPoolVk;
	class AccelerationStructureBuilder;

	class ResourceManagerVk
	{
	public:
		static const uint32_t ALLOW_ALL_MEMORY_TYPES = 0xFFFFFFFF;

		ResourceManagerVk(VkPhysicalDevice physicalDevice, VkInstance instance, const VkExtensions& extFuncs, SubmissionThreadVk& submissionThread, const QueueDefinitionVk& copyQueue, VkDevice device, size_t pipelineLength);
		~ResourceManagerVk();

		void prepare();
		void flushPreFrameUploads(VkSemaphore semaphoreToWaitBeforeUploads);
		void flushFrameUploads();
		void prepareDownloads();

		BufferHandleVk* createBuffer(const YAPT::BufferDesc& desc, const ResourceStateDescription& initialState, uint32_t owningQueueFamily, const char* name);
		TextureHandleVk* createTexture(const YAPT::TextureDesc& desc, const ResourceStateDescription& initialState, uint32_t owningQueueFamily, const char* name);
		void destroyBuffer(BufferHandleVk* handle);
		void destroyTexture(TextureHandleVk* handle);

		bool createBufferVk(const VkBufferCreateInfo& desc, const VmaAllocationCreateInfo& allocInfo, uint32_t owningQueueFamily, VkBuffer* buffOut, Allocation* allocOut, VmaAllocationInfo* infoOut = nullptr);
		bool createTextureVk(const VkImageCreateInfo& desc, const VmaAllocationCreateInfo& allocInfo, uint32_t owningQueueFamily, VkImage* imageOut, Allocation* allocOut, VmaAllocationInfo* infoOut = nullptr);

		void destroyBufferVk(VkBuffer buff, Allocation alloc);
		void destroyTextureVk(VkImage image, Allocation alloc);

		void* map(Allocation alloc);
		void unmap(Allocation alloc);

		VkDevice getDevice() const { return m_device; }

		void upload(VkBuffer handle, uint32_t owningQueueFamilyIndex, size_t offsetInBytes, size_t sizeInBytes, const void* data, GpuUploadStage heapType);
		void upload(BufferHandleVk* handle, size_t offsetInBytes, size_t sizeInBytes, const void* data, GpuUploadStage heapType);
		void* mapUpload(BufferHandleVk* handle, size_t offsetInBytes, size_t sizeInBytes, GpuUploadStage heapType);
		void unmapUpload(BufferHandleVk* handle);
		//direct mapping of the resource memory, nullptr if the resource memory is not host visible.
		//unmap takes the same range that was passed to map
		void* map(BufferHandleVk* handle, size_t offsetInBytes, size_t sizeInBytes);
		void unmap(BufferHandleVk* handle, size_t offsetInBytes, size_t sizeInBytes);
		void* map(TextureHandleVk* handle, uint32_t arraySliceOffset, uint32_t mipOffset);
		void unmap(TextureHandleVk* handle, uint32_t arraySliceOffset, uint32_t mipOffset);
		//unmaps the whole buffer, used by the upload heap path
		
		void upload(TextureHandleVk* image, size_t arraySliceOffset, size_t arraySliceCount, size_t mipOffset, size_t mipCount, const TextureDataDefinition* textureDataDefinitions, GpuUploadStage heapType, const ResourceStateDescription& afterUploadUsage);

		void readback(ReadbackDefinitions& def, GpuDownloadStage stage, FenceHandle fenceToSignal);
		bool hasPendingDownloads() const;
		bool flushDownloads(VkSemaphore* semaphoresToWait, size_t semaphoresToWaitCount, VkSemaphore& signaledSemaphore);

		FenceHandle acquireFence(FenceType type);
		FenceState getFenceState(FenceHandle handle);
		void freeFence(FenceHandle handle);

		ShaderModuleHandle createShaderModule(const char* filepath, ShaderModuleType moduleType, const char* entryPoint, const ShaderModuleDefine* defines, size_t defineCount);
		void destroyShaderModule(ShaderModuleHandle m);

		CommandBufferPoolVk* createCommandBufferPool(size_t numberOfBuffersPerFrame, size_t queueId);
		void destroyCommandBufferPool(CommandBufferPoolVk* pool);

		VkFence getFence(FenceHandle handle) { return m_primitiveMngr.getFence(handle); }

		const QueueDefinitionVk& getCopyQueue() const { return m_copyQueue; }
		AccelerationStructureBuilder* getAccelerationStructureBuilder() { return m_accStructBuilder; }

		VkDeviceAddress GetDeviceAddress(VkBuffer buff)
		{
			VkBufferDeviceAddressInfo info{};
			info.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
			info.buffer = buff;
			return vkGetBufferDeviceAddress(m_device, &info);
		}

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
			entry.cb = [](VkDevice device, const VkExtensions& extensions, char data[32], const VkAllocationCallbacks* alloc)
			{
				T t;
				memcpy(&t, data, sizeof(T));
				destroyVkDeviceChild(device, extensions, t, alloc);
			};

			std::unique_lock<std::mutex> lock(m_destroyObjectsLock);
			m_pendingDestroyedObjects[m_destroyObjectsIndex].push_back(entry);
		}

		inline void deferredFreeAlloc(Allocation alloc)
		{
			std::unique_lock<std::mutex> lock(m_pendingFreeLock);
			m_pendingFreeAlloc[m_destroyObjectsIndex].push_back(alloc);
		}

		VkExtensions& getVkExtFuncs() { return m_extensionFuncs; }
	private:

		typedef void (*DestroyVkResourceCallback)(VkDevice device, const VkExtensions& extensions, char data[32], const VkAllocationCallbacks* alloc);

		struct DestroyResourceEntry
		{
			DestroyVkResourceCallback cb;
			char data[32];
		};

		class SyncImpl
		{
		public:
			SyncImpl(VkDevice device)
				:m_device(device)
			{

			}

			VkFence createFence()
			{
				VkFenceCreateInfo createInfo;
				createInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
				createInfo.pNext = nullptr;
				createInfo.flags = 0;
				VkFence fence;
				VkResult res = vkCreateFence(m_device, &createInfo, VK_ALLOC_CB, &fence);
				assert(checkVkResult(res));
				return fence;
			}

			void destroyFence(VkFence f)
			{
				vkDestroyFence(m_device, f, VK_ALLOC_CB);
			}

			void resetFence(VkFence f)
			{
				vkResetFences(m_device, 1, &f);
			}

			bool isFencePending(VkFence fence)
			{
				VkResult res = vkGetFenceStatus(m_device, fence);
				if (res == VK_NOT_READY) {
					return true;
				}
				return false;
			}

		private:
			VkDevice m_device;
		};

		static constexpr uint32_t MAX_FENCE_COUNT = 1024;
		typedef SyncPrimitiveManager<VkFence, SyncImpl, MAX_FENCE_COUNT> PrimitiveManagerVk;

		void copyViaUploadHeap(VkBuffer handle, uint32_t owningFamilyIndex, size_t offsetInBytes, size_t sizeInBytes, const void* data, GpuUploadStage heapType);
		void copyViaUploadHeap(BufferHandleVk* buffer, size_t offsetInBytes, size_t sizeInBytes, const void* data, GpuUploadStage heapType);
		void copyViaUploadHeap(TextureHandleVk* image, size_t arraySliceOffset, size_t arraySliceCount,
			size_t mipOffset, size_t mipCount, const TextureDataDefinition* textureDataDefinitions, GpuUploadStage heapType, const ResourceStateDescription& afterUploadUsage);
		void* mapCopyRangeFromUploadHeap(BufferHandleVk* buffer, size_t offsetInBytes, size_t sizeInBytes, GpuUploadStage heapType);

		//host cache maintenance around a direct mapping, both nops on host coherent memory
		void invalidateMappedRange(Allocation alloc, MemoryType memoryType, VkDeviceSize offsetInBytes, VkDeviceSize sizeInBytes);
		void flushMappedRange(Allocation alloc, MemoryType memoryType, VkDeviceSize offsetInBytes, VkDeviceSize sizeInBytes);
		//the memory range a single subresource occupies, only valid for linearly tiled images
		VkSubresourceLayout getSubresourceLayout(TextureHandleVk* handle, uint32_t arraySliceOffset, uint32_t mipOffset);

		
		
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
		DownloadHelperVk* m_afterRenderDownloads;

		AccelerationStructureBuilder* m_accStructBuilder;
		PrimitiveManagerVk m_primitiveMngr;
		
		VkExtensions m_extensionFuncs;
		VmaAllocator m_allocator;


		//pending destruction lists
		std::vector<std::vector<DestroyResourceEntry>> m_pendingDestroyedObjects;
		std::vector<std::vector<Allocation>> m_pendingFreeAlloc;
		std::mutex m_destroyObjectsLock;
		std::mutex m_pendingFreeLock;
		size_t m_destroyObjectsIndex;

	};
}
#endif