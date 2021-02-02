#include <Renderer/Vk/ResourceManagerVk.h>
#include <Renderer/Vk/SubmissionThreadVk.h>
#include <Renderer/Vk/UploadHelperVk.h>
#include <Renderer/Vk/ResourceHandlesVk.h>
#include <Renderer/Vk/ShaderModuleVk.h>
#include <assert.h>


namespace YAPT
{
	ResourceManagerVk::ResourceManagerVk(VkPhysicalDevice physicalDevice, SubmissionThreadVk& submissionThread, const QueueDefinitionVk& copyQueue, VkDevice device, size_t pipelineLength)
		:m_submissionThread(submissionThread),
		m_copyQueue(copyQueue),
		m_pipelineLength(pipelineLength),
		m_device(device),
		m_physicalDevice(physicalDevice),
		m_preFrameUploads(nullptr),
		m_duringFrameUploads(nullptr),
		m_destroyObjectsIndex(0)
	{
		m_pendingDestroyedObjects.resize(pipelineLength);
		vkGetPhysicalDeviceMemoryProperties(physicalDevice, &m_memoryProps);

		m_preFrameUploads = new UploadHelperVk(*this, submissionThread, size_t(DEFAULT_UPLOAD_HEAP_SIZE), pipelineLength);
		m_duringFrameUploads = new UploadHelperVk(*this, submissionThread, size_t(DEFAULT_UPLOAD_HEAP_SIZE), pipelineLength);
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
	}

	void ResourceManagerVk::prepare(VkSemaphore semaphoreToWaitBeforeUploads)
	{
		m_preFrameUploads->prepareNextUploadBatch();
		{
			std::unique_lock<std::mutex> lock(m_destroyObjectsMutex);
			m_destroyObjectsIndex = (m_destroyObjectsIndex + 1) % m_pendingDestroyedObjects.size();;
			m_pendingDestroyedObjects[m_destroyObjectsIndex].clear();
		}

		m_lastSignaledSemaphore = semaphoreToWaitBeforeUploads;
	}
	void ResourceManagerVk::flushPreFrameUploads()
	{
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

	bool ResourceManagerVk::allocateDeviceMemory(uint32_t allowedMemoryTypes, VkMemoryPropertyFlags requiredFlags, size_t size, AllocatedMemoryInfo& out)
	{
		for (uint32_t i = 0; i < m_memoryProps.memoryTypeCount; ++i)
		{
			if ((allowedMemoryTypes & (1 << i)) && ((m_memoryProps.memoryTypes[i].propertyFlags & requiredFlags) == requiredFlags))
			{
				VkMemoryAllocateInfo info{};
				info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
				info.allocationSize = size;
				info.memoryTypeIndex = i;

				VkDeviceMemory memory;
				VkResult res = vkAllocateMemory(m_device, &info, VK_ALLOC_CB, &memory);
				if (res == VK_SUCCESS)
				{
					out.memory = memory;
					out.flags = m_memoryProps.memoryTypes[i].propertyFlags;
					return true;
				}
			}
		}

		return false;
	}


	void ResourceManagerVk::upload(BufferHandleVk* handle, size_t offsetInBytes, size_t sizeInBytes, const void* data, GpuUploadStage heapType)
	{
		if ((handle->memoryFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) == 0)
		{
			copyViaUploadHeap(handle->buffer, offsetInBytes, sizeInBytes, data, heapType);
		}
		else
		{
			memcpy(handle->mappedMemory + offsetInBytes, data, sizeInBytes);
			unmap(handle);
		}
	}
	void* ResourceManagerVk::map(BufferHandleVk* handle, size_t offsetInBytes, size_t sizeInBytes, GpuUploadStage heapType)
	{
		if ((handle->memoryFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) == 0)
		{
			return mapCopyRangeFromUploadHeap(handle->buffer, offsetInBytes, sizeInBytes, heapType);
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

	void ResourceManagerVk::upload(TextureHandleVk* image, size_t arraySliceOffset, size_t arraySliceCount, size_t mipOffset, size_t mipCount, const TextureDataDefinition* textureDataDefinitions, GpuUploadStage heapType)
	{

		if ((image->memoryFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) == 0)
		{
			copyViaUploadHeap(image, arraySliceOffset, arraySliceCount, mipOffset, mipCount, textureDataDefinitions, heapType);
		}
		else
		{
			YAPT_LOG_FATAL_ERROR("upload via direct map not yet implemented");
		}
	}

	void* ResourceManagerVk::mapCopyRangeFromUploadHeap(VkBuffer buffer, size_t offsetInBytes, size_t sizeInBytes, GpuUploadStage heapType)
	{
		switch (heapType)
		{
		case YAPT::GpuUploadStage::BEFORE_RENDER:
		{
			return m_preFrameUploads->mapCopyRangeForBufferData(buffer, offsetInBytes, sizeInBytes);
			break;
		}
		case YAPT::GpuUploadStage::DURING_RENDER:
		{
			return m_duringFrameUploads->mapCopyRangeForBufferData(buffer, offsetInBytes, sizeInBytes);
			break;
		}
		default:
			assert(!"unknown upload stage");
			break;
		}
		return nullptr;
	}

	void ResourceManagerVk::copyViaUploadHeap(VkBuffer buffer, size_t offsetInBytes, size_t sizeInBytes, const void* data, GpuUploadStage heapType)
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


	void ResourceManagerVk::copyViaUploadHeap(TextureHandleVk* image, size_t arraySliceOffset, size_t arraySliceCount, size_t mipOffset, size_t mipCount, const TextureDataDefinition* textureDataDefinitions, GpuUploadStage heapType)
	{
		VkImageLayout currentLayout = image->currentLayout;
		VkImageLayout afterCopyLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		image->currentLayout = afterCopyLayout;
		switch (heapType)
		{
		case YAPT::GpuUploadStage::BEFORE_RENDER:
		{
			m_preFrameUploads->uploadDataForTexture(image->image, image->createInfo, currentLayout, afterCopyLayout, arraySliceOffset, arraySliceCount, mipOffset, mipCount, textureDataDefinitions);
			break;
		}
		case YAPT::GpuUploadStage::DURING_RENDER:
		{
			m_duringFrameUploads->uploadDataForTexture(image->image, image->createInfo, currentLayout, afterCopyLayout, arraySliceOffset, arraySliceCount, mipOffset, mipCount, textureDataDefinitions);
			break;
		}
		default:
			assert(!"unknown upload stage");
			break;
		}
	}


	ShaderModuleHandle ResourceManagerVk::createShaderModule(const char* filepath, ShaderModuleType moduleType, const char* entryPoint, const ShaderModuleDefine* defines, size_t defineCount)
	{
		ShaderModuleHandle mod = new ShaderModuleVk(filepath, moduleType, entryPoint, defines, defineCount);
		assert("TODO");
		/*for (size_t i = 0; i < defineCount; ++i)
		{
			stringToWString(defines[i].name, defineWstr[i * 2]);

			definesDxc[i].Name = defineWstr[i * 2].c_str();
			if (defines[i].value != nullptr)
			{
				stringToWString(defines[i].value, defineWstr[i * 2 + 1]);
				definesDxc[i].Value = defineWstr[i * 2 + 1].c_str();
			}
		}

		definesDxc.back().Name = L"VK";
		definesDxc.back().Value = L"1";


		mod->type = moduleType;
		mod->shaderBlob = compileFromFile(profileWStr.data(), entryPointWStr.data(), moduleType == ShaderModuleType::LIBRARY_MODULE ? L"" : entryPointWStr.data(), filePathWStr.data(), definesDxc.data(), (UINT32)defineCountFinal);
		if (mod->shaderBlob.get())
		{
			readReflectionData(mod->shaderBlob.get(), entryPoint, mod->reflection);
		}
		*/
		return mod;
	}
	void ResourceManagerVk::destroyShaderModule(ShaderModuleHandle m)
	{
		delete m;
	}

}