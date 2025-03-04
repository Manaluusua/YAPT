#pragma once

#include <Gfx/GfxApi.h>
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
		void alloc(ResourceManagerVk& mngr, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags memoryPropertyBits);
		void dealloc(ResourceManagerVk& mngr);


		VkBuffer buffer;
		Allocation deviceMemory;
		VkDeviceAddress deviceAddress;
	};

}