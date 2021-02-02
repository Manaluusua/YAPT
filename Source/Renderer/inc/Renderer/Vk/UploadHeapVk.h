#pragma once

#include <Renderer/Vk/CommonVk.h>
#include <Renderer/Shared/GfxTypes.h>
#include <vector>
#include <atomic>

namespace YAPT
{
	class ResourceManagerVk;

	class UploadHeapVk
	{
	public:

		struct UploadHeapAllocationInfo
		{
			size_t offsetToHeap;
			VkBuffer uploadBuffer;
			char* mappedPtr;
		};

		UploadHeapVk(ResourceManagerVk& resourceMngr, size_t heapSize, size_t numberOfPartitions);
		~UploadHeapVk();

		UploadHeapVk(const UploadHeapVk&) = delete;

		void nextPartition();

		bool allocate(size_t size, UploadHeapAllocationInfo& heapAllocation);

	private:

		//upload heap stuff
		ResourceManagerVk& m_resMngr;
		VkBuffer m_uploadBuffer;
		VkDeviceMemory m_deviceMemory;
		char* m_mappedUploadBufferPtr;
		size_t m_heapSize;
		size_t m_numberOfPartitions;

		std::atomic<size_t> m_currentlyUsedPartitionSize;
		size_t m_sizePerPartition;
		size_t m_currentPartitionIndex;
	};

	










}