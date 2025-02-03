#include <Gfx/Vk/SimpleBufferAllocationUtility.h>
#include <Gfx/Vk/ResourceManagerVk.h>
namespace YAPT
{
	void SimpleBufferAllocationUtility::alloc(ResourceManagerVk& mngr, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags memoryPropertyFlags, bool needsDeviceAddress)
	{
		assert(buffer == VK_NULL_HANDLE);
		assert(deviceMemory == VK_NULL_HANDLE);

		VkBufferCreateInfo buffCreateInfo{};
		buffCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		buffCreateInfo.size = size;
		buffCreateInfo.usage = usage;
		buffCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		VkResult res = vkCreateBuffer(mngr.getDevice(), &buffCreateInfo, VK_ALLOC_CB, &buffer);

		ResourceManagerVk::AllocatedMemoryInfo memInfo;
		bool success = mngr.allocateDeviceMemory(buffer, memoryPropertyFlags, needsDeviceAddress, memInfo);
		deviceMemory = memInfo.memory;

		res = vkBindBufferMemory(mngr.getDevice(), buffer, deviceMemory, 0);

		checkVkResult(res);
		assert(success);

		deviceAddress = mngr.GetDeviceAddress(buffer);
	}
	void SimpleBufferAllocationUtility::dealloc(ResourceManagerVk& mngr)
	{
		assert(buffer != VK_NULL_HANDLE);
		assert(deviceMemory != VK_NULL_HANDLE);

		mngr.deferredDestroyVkResource(buffer);
		mngr.deferredDestroyVkResource(deviceMemory);

		buffer = VK_NULL_HANDLE;
		deviceMemory = VK_NULL_HANDLE;
		deviceAddress = NULL;

	}
}