#include <Gfx/Vk/ResourceManagerVk.h>
#include <Gfx/Vk/SubmissionThreadVk.h>
#include <Gfx/Vk/UploadHelperVk.h>
#include <Gfx/Vk/ResourceHandlesVk.h>
#include <Gfx/Vk/ShaderModuleVk.h>
#include <Gfx/Vk/YaptToVkConversions.h>
#include <Gfx/Vk/AccelerationStructureBuilderVk.h>

#include <assert.h>
#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"


namespace YAPT
{

	

	ResourceManagerVk::ResourceManagerVk(VkPhysicalDevice physicalDevice, VkInstance instance, const VkExtensions& extFuncs, SubmissionThreadVk& submissionThread, const QueueDefinitionVk& copyQueue, VkDevice device, size_t pipelineLength)
		:m_submissionThread(submissionThread),
		m_copyQueue(copyQueue),
		m_pipelineLength(pipelineLength),
		m_device(device),
		m_physicalDevice(physicalDevice),
		m_accStructBuilder(nullptr),
		m_primitiveMngr(device),
		m_extensionFuncs(extFuncs),
		m_preFrameUploads(nullptr),
		m_duringFrameUploads(nullptr),
		m_destroyObjectsIndex(0)
	{
		m_pendingDestroyedObjects.resize(pipelineLength);
		m_pendingFreeAlloc.resize(pipelineLength);
		vkGetPhysicalDeviceMemoryProperties(physicalDevice, &m_memoryProps);

		//init vma
		VmaVulkanFunctions vulkanFunctions = {};
		vulkanFunctions.vkGetInstanceProcAddr = &vkGetInstanceProcAddr;
		vulkanFunctions.vkGetDeviceProcAddr = &vkGetDeviceProcAddr;
		VmaAllocatorCreateInfo allocatorCreateInfo = {};
		allocatorCreateInfo.flags = VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT | VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
		allocatorCreateInfo.vulkanApiVersion = VK_API_VERSION_1_2;
		allocatorCreateInfo.physicalDevice = physicalDevice;
		allocatorCreateInfo.device = device;
		allocatorCreateInfo.instance = instance;
		allocatorCreateInfo.pVulkanFunctions = &vulkanFunctions;

		vmaCreateAllocator(&allocatorCreateInfo, &m_allocator);

		m_preFrameUploads = new UploadHelperVk(*this, submissionThread, size_t(DEFAULT_UPLOAD_HEAP_SIZE), pipelineLength);
		m_duringFrameUploads = new UploadHelperVk(*this, submissionThread, size_t(DEFAULT_UPLOAD_HEAP_SIZE), pipelineLength);
		m_accStructBuilder = new AccelerationStructureBuilder(*this);
		m_lastSignaledSemaphore = VK_NULL_HANDLE;

		
	}
	ResourceManagerVk::~ResourceManagerVk()
	{
		deinitialize();
		
	}

	void ResourceManagerVk::deinitialize()
	{
		delete m_preFrameUploads;
		m_preFrameUploads = nullptr;
		delete m_duringFrameUploads;
		m_duringFrameUploads = nullptr;
		delete m_accStructBuilder;
		m_accStructBuilder = nullptr;

		//destroy pending list
		{
			std::unique_lock<std::mutex> lock(m_destroyObjectsLock);
			for (size_t i = 0; i < m_pendingDestroyedObjects.size(); ++i)
			{
				std::vector<DestroyResourceEntry>& destroyList = m_pendingDestroyedObjects[i];
				for (size_t k = 0; k < destroyList.size(); ++k)
				{
					destroyList[k].cb(m_device, m_extensionFuncs, destroyList[k].data, VK_ALLOC_CB);
				}
				destroyList.clear();
			}

		}

		//destroy pending list
		{
			std::unique_lock<std::mutex> lock(m_pendingFreeLock);
			for (size_t i = 0; i < m_pendingFreeAlloc.size(); ++i)
			{
				std::vector<Allocation>& freeList = m_pendingFreeAlloc[i];
				for (size_t k = 0; k < freeList.size(); ++k)
				{
					vmaFreeMemory(m_allocator, freeList[k]);
				}
				freeList.clear();
			}

		}
		vmaDestroyAllocator(m_allocator);

	}

	void ResourceManagerVk::prepare()
	{
		m_preFrameUploads->prepareNextUploadBatch();
		{
			//apply & clear pending destruction list
			m_destroyObjectsIndex = (m_destroyObjectsIndex + 1) % m_pendingDestroyedObjects.size();
			{
				std::unique_lock<std::mutex> lock(m_destroyObjectsLock);
				std::vector<DestroyResourceEntry>& destroyList = m_pendingDestroyedObjects[m_destroyObjectsIndex];
				for (size_t i = 0; i < destroyList.size(); ++i)
				{
					destroyList[i].cb(m_device, m_extensionFuncs, destroyList[i].data, VK_ALLOC_CB);
				}
				destroyList.clear();
			}
		}

		{
			std::unique_lock<std::mutex> lock(m_pendingFreeLock);
			std::vector<Allocation>& freeList = m_pendingFreeAlloc[m_destroyObjectsIndex];
			for (size_t k = 0; k < freeList.size(); ++k)
			{
				vmaFreeMemory(m_allocator, freeList[k]);
			}
			freeList.clear();
		}
		
	}
	void ResourceManagerVk::flushPreFrameUploads(VkSemaphore semaphoreToWaitBeforeUploads)
	{

		m_lastSignaledSemaphore = semaphoreToWaitBeforeUploads;
		VkSemaphore signaledSemaphore;
		if (m_preFrameUploads->flushUploadBatch(&m_lastSignaledSemaphore, m_lastSignaledSemaphore != VK_NULL_HANDLE ? 1 : 0, signaledSemaphore))
		{
			m_lastSignaledSemaphore = signaledSemaphore;
		}
		m_duringFrameUploads->prepareNextUploadBatch();

	}
	void ResourceManagerVk::flushFrameUploads()
	{
		VkSemaphore signaledSemaphore;
		if (m_duringFrameUploads->flushUploadBatch(&m_lastSignaledSemaphore, m_lastSignaledSemaphore != VK_NULL_HANDLE ? 1 : 0, signaledSemaphore))
		{
			m_lastSignaledSemaphore = signaledSemaphore;
		}
	}


	void ResourceManagerVk::upload(VkBuffer handle, uint32_t owningQueueFamilyIndex, size_t offsetInBytes, size_t sizeInBytes, const void* data, GpuUploadStage heapType)
	{
		copyViaUploadHeap(handle, owningQueueFamilyIndex, offsetInBytes, sizeInBytes, data, heapType);
	}


	void ResourceManagerVk::upload(BufferHandleVk* handle, size_t offsetInBytes, size_t sizeInBytes, const void* data, GpuUploadStage heapType)
	{
		if (!handle->mappable)
		{
			copyViaUploadHeap(handle, offsetInBytes, sizeInBytes, data, heapType);
		}
		else
		{
			memcpy(handle->mappedMemory + offsetInBytes, data, sizeInBytes);
			unmap(handle);
		}
	}
	void* ResourceManagerVk::map(BufferHandleVk* handle, size_t offsetInBytes, size_t sizeInBytes, GpuUploadStage heapType)
	{
		if (!handle->mappable)
		{
			return mapCopyRangeFromUploadHeap(handle, offsetInBytes, sizeInBytes, heapType);
		}
		else
		{
			return handle->mappedMemory + offsetInBytes;
		}

	}

	void ResourceManagerVk::unmap(BufferHandleVk* handle)
	{
		//for now we do nothing. TODO: invalidate caches if the memory is not host coherent 
	}

	void ResourceManagerVk::upload(TextureHandleVk* image, size_t arraySliceOffset, size_t arraySliceCount, size_t mipOffset, size_t mipCount, const TextureDataDefinition* textureDataDefinitions, GpuUploadStage heapType, const ResourceStateDescription& afterUploadUsage)
	{

		if(!image->mappable)
		{
			copyViaUploadHeap(image, arraySliceOffset, arraySliceCount, mipOffset, mipCount, textureDataDefinitions, heapType, afterUploadUsage);
		}
		else
		{
			YAPT_LOG_FATAL_ERROR("upload via direct map not yet implemented");
		}
	}

	void* ResourceManagerVk::mapCopyRangeFromUploadHeap(BufferHandleVk* buffer, size_t offsetInBytes, size_t sizeInBytes, GpuUploadStage heapType)
	{
		switch (heapType)
		{
		case YAPT::GpuUploadStage::BEFORE_RENDER:
		{
			return m_preFrameUploads->mapCopyRangeForBufferData(buffer->buffer, buffer->owningQueueFamily, offsetInBytes, sizeInBytes);
			break;
		}
		case YAPT::GpuUploadStage::DURING_RENDER:
		{
			return m_duringFrameUploads->mapCopyRangeForBufferData(buffer->buffer, buffer->owningQueueFamily, offsetInBytes, sizeInBytes);
			break;
		}
		default:
			assert(!"unknown upload stage");
			break;
		}
		return nullptr;
	}

	void ResourceManagerVk::copyViaUploadHeap(VkBuffer handle, uint32_t owningFamilyIndex, size_t offsetInBytes, size_t sizeInBytes, const void* data, GpuUploadStage heapType)
	{
		switch (heapType)
		{
		case YAPT::GpuUploadStage::BEFORE_RENDER:
		{
			m_preFrameUploads->uploadDataForBuffer(handle, owningFamilyIndex, offsetInBytes, sizeInBytes, data);
			break;
		}
		case YAPT::GpuUploadStage::DURING_RENDER:
		{
			m_duringFrameUploads->uploadDataForBuffer(handle, owningFamilyIndex, offsetInBytes, sizeInBytes, data);
			break;
		}
		default:
			assert(!"unknown upload stage");
			break;
		}
	}

	void ResourceManagerVk::copyViaUploadHeap(BufferHandleVk* buffer, size_t offsetInBytes, size_t sizeInBytes, const void* data, GpuUploadStage heapType)
	{
		switch (heapType)
		{
		case YAPT::GpuUploadStage::BEFORE_RENDER:
		{
			m_preFrameUploads->uploadDataForBuffer(buffer, offsetInBytes, sizeInBytes, data);
			break;
		}
		case YAPT::GpuUploadStage::DURING_RENDER:
		{
			m_duringFrameUploads->uploadDataForBuffer(buffer, offsetInBytes, sizeInBytes, data);
			break;
		}
		default:
			assert(!"unknown upload stage");
			break;
		}
	}


	void ResourceManagerVk::copyViaUploadHeap(TextureHandleVk* image, size_t arraySliceOffset, size_t arraySliceCount, size_t mipOffset, size_t mipCount, const TextureDataDefinition* textureDataDefinitions, GpuUploadStage heapType, const ResourceStateDescription& afterUploadUsage)
	{

		switch (heapType)
		{
		case YAPT::GpuUploadStage::BEFORE_RENDER:
		{
			m_preFrameUploads->uploadDataForTexture(image, image->createInfo, arraySliceOffset, arraySliceCount, mipOffset, mipCount, textureDataDefinitions, afterUploadUsage);
			break;
		}
		case YAPT::GpuUploadStage::DURING_RENDER:
		{
			m_duringFrameUploads->uploadDataForTexture(image, image->createInfo, arraySliceOffset, arraySliceCount, mipOffset, mipCount, textureDataDefinitions, afterUploadUsage);
			break;
		}
		default:
			assert(!"unknown upload stage");
			break;
		}
	}
	 
	BufferHandleVk* ResourceManagerVk::createBuffer(const YAPT::BufferDesc& desc, const ResourceStateDescription& initialState, uint32_t owningQueueFamily, const char* name)
	{
		BufferHandleVk* buffHandle = new BufferHandleVk(*this);
		yaptBufferDescToVk(desc, buffHandle->createInfo);
#ifdef VK_DEBUGNAMES_ENABLE
		if (name)
		{
			buffHandle->name = std::string(name);
		}
#endif
		VmaAllocationCreateInfo createInfo{};
		createInfo.usage = VMA_MEMORY_USAGE_AUTO;
		createInfo.flags = desc.memoryType == MemoryType::DEFAULT ? 0 : VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;
		bool res = createBufferVk(buffHandle->createInfo, createInfo, owningQueueFamily, &buffHandle->buffer, &buffHandle->alloc, nullptr);

		buffHandle->owningQueueFamily = owningQueueFamily;
		buffHandle->lastUsedStages = VK_PIPELINE_STAGE_NONE;
		buffHandle->mappable = desc.memoryType == MemoryType::DEFAULT ? false : true;
		buffHandle->buffDesc = desc;
		
		if (!res)
		{
			delete buffHandle;
			buffHandle = nullptr;
		}
		return buffHandle;
	}

	TextureHandleVk* ResourceManagerVk::createTexture(const YAPT::TextureDesc& desc, const ResourceStateDescription& initialState, uint32_t owningQueueFamily, const char* name)
	{
		TextureHandleVk* texHandle = new TextureHandleVk(*this);
		yaptTextureDescToVk(desc, texHandle->createInfo, desc.memoryType == MemoryType::CPU_MAPPABLE_READBACK);
		texHandle->currentLayouts.init(VK_IMAGE_LAYOUT_UNDEFINED, desc.depthOrSlices * desc.mips);
		texHandle->owningQueueFamily = owningQueueFamily;
		texHandle->lastUsedStages = VK_PIPELINE_STAGE_NONE;
		texHandle->mappable = desc.memoryType == MemoryType::DEFAULT ? false : true;
		texHandle->texDesc = desc;
#ifdef VK_DEBUGNAMES_ENABLE
		if (name)
		{
			texHandle->name = std::string(name);
		}
#endif

		VmaAllocationCreateInfo createInfo{};
		createInfo.usage = VMA_MEMORY_USAGE_AUTO;
		createInfo.flags = desc.memoryType == MemoryType::DEFAULT ? 0 : VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;
		bool res = createTextureVk(texHandle->createInfo, createInfo, owningQueueFamily, &texHandle->image, &texHandle->alloc, nullptr);
		if (!res)
		{
			delete texHandle;
			texHandle = nullptr;
		}

		return texHandle;
	}

	void ResourceManagerVk::destroyBuffer(BufferHandleVk* handle)
	{
		destroyBufferVk(handle->buffer, handle->alloc);
		delete handle;
	}

	void ResourceManagerVk::destroyTexture(TextureHandleVk* handle)
	{
		destroyTextureVk(handle->image, handle->alloc);
		delete handle;
	}

	bool ResourceManagerVk::createBufferVk(const VkBufferCreateInfo& desc, const VmaAllocationCreateInfo& allocInfo, uint32_t owningQueueFamily, VkBuffer* buffOut, Allocation* allocOut, VmaAllocationInfo* infoOut)
	{
		VkResult res = vmaCreateBuffer(m_allocator, &desc, &allocInfo, buffOut, allocOut, infoOut);
		return checkVkResult(res);
	}

	bool ResourceManagerVk::createTextureVk(const VkImageCreateInfo& desc, const VmaAllocationCreateInfo& allocInfo, uint32_t owningQueueFamily, VkImage* imageOut, Allocation* allocOut, VmaAllocationInfo* infoOut)
	{
		VkResult res = vmaCreateImage(m_allocator, &desc, &allocInfo, imageOut, allocOut, infoOut);
		return checkVkResult(res);
	}

	void* ResourceManagerVk::map(Allocation alloc)
	{
		
		if (alloc->IsPersistentMap())
		{
			return alloc->GetMappedData();
		}
		else
		{
			void* mapped;
			vmaMapMemory(m_allocator, alloc, &mapped);
			return mapped;
		}
	}
	void ResourceManagerVk::unmap(Allocation alloc)
	{
		if (!alloc->IsPersistentMap())
		{
			vmaUnmapMemory(m_allocator, alloc);
		}
	}
	

	void ResourceManagerVk::destroyBufferVk(VkBuffer buff, Allocation alloc)
	{
		vmaFreeMemory(m_allocator, alloc);
		deferredDestroyVkResource(buff);
	}

	void ResourceManagerVk::destroyTextureVk(VkImage image, Allocation alloc)
	{
		vmaFreeMemory(m_allocator, alloc);
		deferredDestroyVkResource(image);
	}


	ShaderModuleHandle ResourceManagerVk::createShaderModule(const char* filepath, ShaderModuleType moduleType, const char* entryPoint, const ShaderModuleDefine* defines, size_t defineCount)
	{
		ShaderModuleVk* mod = new ShaderModuleVk();
		bool success = mod->compileFromHLSL(m_device, filepath, moduleType, entryPoint, defines, defineCount);
		assert(success);
		return mod;
	}
	void ResourceManagerVk::destroyShaderModule(ShaderModuleHandle m)
	{
		deferredDestroyVkResource(m->m_vkShaderModule);
		delete m;
	}

	CommandBufferPoolVk* ResourceManagerVk::createCommandBufferPool(size_t numberOfBuffersPerFrame, size_t queueId)
	{
		CommandBufferPoolVk* pool = new CommandBufferPoolVk(m_device);
		pool->initialize(m_pipelineLength, numberOfBuffersPerFrame, (uint32_t)queueId);

		
		return pool;
	}
	void ResourceManagerVk::destroyCommandBufferPool(CommandBufferPoolVk* pool)
	{
		pool->deinitialize();
		delete pool;
	}

}