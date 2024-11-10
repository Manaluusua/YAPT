#pragma once

#include <Renderer/Vk/CommonVk.h>
#include <assert.h>

namespace YAPT
{
	class ResourceManagerVk;
	struct SimpleBufferAllocationUtility
	{
		SimpleBufferAllocationUtility()
			:buffer(VK_NULL_HANDLE),
			deviceMemory(VK_NULL_HANDLE),
			deviceAddress(NULL)
		{
		}

		//Simple Buffer Allocation Utility
		void alloc(ResourceManagerVk& mngr, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags memoryPropertyBits, bool needsDeviceAddress = true);
		void dealloc(ResourceManagerVk& mngr);


		VkBuffer buffer;
		VkDeviceMemory deviceMemory;
		VkDeviceAddress deviceAddress;
	};

}