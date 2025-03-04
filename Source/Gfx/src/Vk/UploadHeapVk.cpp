#include <Gfx/Vk/UploadHeapVk.h>
#include <Gfx/Vk/ResourceManagerVk.h>


namespace YAPT
{
	UploadHeapVk::UploadHeapVk(ResourceManagerVk& resourceMngr, size_t heapSize, size_t numberOfPartitions)
		:m_resMngr(resourceMngr),
		m_uploadBuffer(VK_NULL_HANDLE),
		m_alloc(nullptr),
		m_heapSize(heapSize),
		m_numberOfPartitions(numberOfPartitions),
		m_currentlyUsedPartitionSize(0),
		m_sizePerPartition(heapSize / numberOfPartitions),
		m_currentPartitionIndex(0)
	{

		VkBufferCreateInfo buffCreateInfo{};
		buffCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		buffCreateInfo.size = heapSize;
		buffCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		buffCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		VmaAllocationCreateInfo allocInfo = {};
		allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
		allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;// VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;


		bool success = m_resMngr.createBufferVk(buffCreateInfo, allocInfo, 0, &m_uploadBuffer, &m_alloc);

		assert(success);

		m_mappedUploadBufferPtr = (char*)m_resMngr.map(m_alloc);

	}
	UploadHeapVk::~UploadHeapVk()
	{
		m_resMngr.unmap(m_alloc);
		m_resMngr.destroyBufferVk(m_uploadBuffer, m_alloc);
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