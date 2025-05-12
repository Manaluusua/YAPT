#include <Gfx/Vk/SimpleBufferAllocationUtility.h>
#include <Gfx/Vk/ResourceManagerVk.h>
namespace YAPT
{
	void SimpleBufferAllocationUtility::alloc(ResourceManagerVk& mngr, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags memoryPropertyFlags)
	{

		if (buffer != VK_NULL_HANDLE)
		{
			dealloc(mngr);
		}

		VkBufferCreateInfo buffCreateInfo{};
		buffCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		buffCreateInfo.size = size;
		buffCreateInfo.usage = usage;
		buffCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		VmaAllocationCreateInfo allocInfo = {};
		allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
		allocInfo.requiredFlags = memoryPropertyFlags;

		if (memoryPropertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
		{
			allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT;
		}

		bool success = mngr.createBufferVk(buffCreateInfo, allocInfo, 0, &buffer, &deviceMemory);

		assert(success);

		deviceAddress = mngr.GetDeviceAddress(buffer);

		vkGetBufferMemoryRequirements(mngr.getDevice(), buffer, &memoryRequirements);
	}
	void SimpleBufferAllocationUtility::dealloc(ResourceManagerVk& mngr)
	{

		if (buffer != VK_NULL_HANDLE)
		{
			mngr.destroyBufferVk(buffer, deviceMemory);
		}

		buffer = VK_NULL_HANDLE;
		deviceMemory = VK_NULL_HANDLE;
		deviceAddress = NULL;

	}
}