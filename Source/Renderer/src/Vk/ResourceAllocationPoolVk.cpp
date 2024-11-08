#include <Renderer/Vk/ResourceAllocationPoolVk.h>
#include <Renderer/Vk/ResourceHandlesVk.h>
#include <unordered_map>

namespace YAPT
{



	ResourceAllocationPoolVk::ResourceAllocationPoolVk(ResourceManagerVk& mngr)
		:m_resourceMngr(mngr)
	{
		
	

	}
	ResourceAllocationPoolVk::~ResourceAllocationPoolVk()
	{
		for (size_t i = 0; i < m_allocatedMemory.size(); ++i)
		{
			if (m_memoryMapped)
			{
				vkUnmapMemory(m_resourceMngr.getDevice(), m_allocatedMemory[i].memory);
			}
			m_resourceMngr.deferredDestroyVkResource(m_allocatedMemory[i].memory);
		}

		m_allocatedMemory.clear();

	}

	bool ResourceAllocationPoolVk::allocate(TextureHandle* textures, size_t textureCount, BufferHandle* buffers, size_t bufferCount, ResourcePoolType type)
	{
		struct SortedMemoryEntries
		{
			SortedMemoryEntries()
				:needsDeviceAddressMemory(false)
			{

			}
			std::vector<size_t> entries;
			bool needsDeviceAddressMemory;
		};

		assert(type == ResourcePoolType::RESOURCEPOOL_TYPE_DEFAULT); //others not implemented
		VkDevice device = m_resourceMngr.getDevice();
		std::vector<VkMemoryRequirements> memReq;
		std::unordered_map<uint32_t, SortedMemoryEntries> sortedByMemoryTypeBits;

		memReq.resize(textureCount + bufferCount);

		VkMemoryPropertyFlags memoryFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

		if (type == ResourcePoolType::RESOURCEPOOL_TYPE_CPU_MAPPABLE_UPLOAD)
		{
			memoryFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
		}
		else if (type == ResourcePoolType::RESOURCEPOOL_TYPE_CPU_MAPPABLE_READBACK)
		{
			memoryFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
		}

		m_memoryMapped = memoryFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;


		for (size_t i = 0; i < textureCount; ++i)
		{
			vkGetImageMemoryRequirements(device, textures[i]->image, &memReq[i]);
			sortedByMemoryTypeBits[memReq[i].memoryTypeBits].entries.push_back(i);
		}

		for (size_t i = 0; i < bufferCount; ++i)
		{
			vkGetBufferMemoryRequirements(device, buffers[i]->buffer, &memReq[textureCount + i]);
			SortedMemoryEntries& entries = sortedByMemoryTypeBits[memReq[i].memoryTypeBits];
			entries.entries.push_back(textureCount + i);
			if (buffers[i]->createInfo.usage & VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR)
			{
				entries.needsDeviceAddressMemory = true;
			}
		}

		std::vector<size_t> offsets;
		for (auto iter = sortedByMemoryTypeBits.begin(); iter != sortedByMemoryTypeBits.end(); ++iter)
		{
			size_t allocationSize = 0;
			std::vector<size_t>& indices = iter->second.entries;
			offsets.resize(indices.size());
			for (size_t i = 0; i < indices.size(); ++i)
			{
				size_t index = indices[i];

				allocationSize = align(allocationSize, memReq[index].alignment);
				offsets[i] = allocationSize;
				allocationSize += memReq[index].size;
			}
			

			ResourceManagerVk::AllocatedMemoryInfo allocation;
			bool success = m_resourceMngr.allocateDeviceMemory(iter->first, memoryFlags, allocationSize, iter->second.needsDeviceAddressMemory, allocation);
			if (!success)
			{
				return false;
			}
			m_allocatedMemory.push_back(allocation);
			char* mappedMemory = nullptr;
			void* ptr = (void*)mappedMemory;
			if (m_memoryMapped)
			{
				vkMapMemory(device, allocation.memory, 0, allocationSize, 0, &ptr);
			}

			for (size_t i = 0; i < indices.size(); ++i)
			{
				size_t index = indices[i];
				size_t offset = offsets[i];
				if (index < textureCount)
				{
					checkForVkError(vkBindImageMemory(device, textures[index]->image, allocation.memory, offset));
					textures[index]->memoryFlags = allocation.flags;
					textures[index]->mappedMemory = m_memoryMapped ? mappedMemory + offset : nullptr;
				}
				else
				{
					checkForVkError(vkBindBufferMemory(device, buffers[index - textureCount]->buffer, allocation.memory, offset));
					buffers[index - textureCount]->memoryFlags = allocation.flags;
					buffers[index - textureCount]->mappedMemory = m_memoryMapped ? mappedMemory + offset : nullptr;
				}
			}
		}

		return true;
	}

}


