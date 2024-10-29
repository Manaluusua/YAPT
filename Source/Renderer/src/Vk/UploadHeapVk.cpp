#include <Renderer/Vk/UploadHeapVk.h>
#include <Renderer/Vk/ResourceManagerVk.h>


namespace YAPT
{
	UploadHeapVk::UploadHeapVk(ResourceManagerVk& resourceMngr, size_t heapSize, size_t numberOfPartitions)
		:m_resMngr(resourceMngr),
		m_uploadBuffer(VK_NULL_HANDLE),
		m_deviceMemory(VK_NULL_HANDLE),
		m_heapSize(heapSize),
		m_numberOfPartitions(numberOfPartitions),
		m_currentlyUsedPartitionSize(0),
		m_sizePerPartition(heapSize / numberOfPartitions),
		m_currentPartitionIndex(0)
	{
		//create upload heap and required resources
		VkBufferCreateInfo buffCreateInfo{};
		buffCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		buffCreateInfo.size = heapSize;
		buffCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		buffCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		VkResult res = vkCreateBuffer(m_resMngr.getDevice(), &buffCreateInfo, VK_ALLOC_CB, &m_uploadBuffer);
		
		ResourceManagerVk::AllocatedMemoryInfo memInfo;
		bool success = resourceMngr.allocateDeviceMemory(0xFFFFFFFF, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, heapSize, false, memInfo);
		m_deviceMemory = memInfo.memory;

		res = vkBindBufferMemory(m_resMngr.getDevice(), m_uploadBuffer, m_deviceMemory, 0);

		checkVkResult(res);
		assert(success);

		void* ptr = (void*)m_mappedUploadBufferPtr;

		vkMapMemory(m_resMngr.getDevice(), m_deviceMemory, 0, heapSize, 0, &ptr);

		m_mappedUploadBufferPtr = (char*)ptr;


	}
	UploadHeapVk::~UploadHeapVk()
	{
		vkUnmapMemory(m_resMngr.getDevice(), m_deviceMemory);

		vkDestroyBuffer(m_resMngr.getDevice(), m_uploadBuffer, VK_ALLOC_CB);
		vkFreeMemory(m_resMngr.getDevice(), m_deviceMemory, VK_ALLOC_CB);
	}


	

	bool UploadHeapVk::allocate(size_t size, UploadHeapVk::UploadHeapAllocationInfo& heapAllocation)
	{
		size_t copyOffset = m_currentPartitionIndex * m_sizePerPartition;

		size_t currentlyUsedPartitionSize;
		while (true)
		{
			currentlyUsedPartitionSize = m_currentlyUsedPartitionSize.load();
			if (currentlyUsedPartitionSize + size > m_sizePerPartition)
			{
				return false;
			}

			if (m_currentlyUsedPartitionSize.compare_exchange_weak(currentlyUsedPartitionSize, currentlyUsedPartitionSize + size))
			{
				break;
			}
		}
		
		heapAllocation.offsetToHeap = copyOffset + currentlyUsedPartitionSize;
		heapAllocation.uploadBuffer = m_uploadBuffer;
		heapAllocation.mappedPtr = m_mappedUploadBufferPtr + heapAllocation.offsetToHeap;

		return true;
	}

	void  UploadHeapVk::nextPartition()
	{
		m_currentPartitionIndex = (m_currentPartitionIndex + 1) % m_numberOfPartitions;
		m_currentlyUsedPartitionSize.store(0);
	}
}