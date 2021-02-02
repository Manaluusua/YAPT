#pragma once

#include <Renderer/Vk/CommonVk.h>

namespace YAPT
{
	template<typename T>
	inline void destroyVkDeviceChild(VkDevice device, T child, const VkAllocationCallbacks* alloc)
	{
		dummyDestroyVkNotImplemented(device, child, alloc);
	}

	template<>
	inline void destroyVkDeviceChild<VkBuffer>(VkDevice device, VkBuffer child, const VkAllocationCallbacks* alloc)
	{
		vkDestroyBuffer(device, child, alloc);
	}

	template<>
	inline void destroyVkDeviceChild<VkImage>(VkDevice device, VkImage child, const VkAllocationCallbacks* alloc)
	{
		vkDestroyImage(device, child, alloc);
	}

	template<>
	inline void destroyVkDeviceChild<VkImageView>(VkDevice device, VkImageView child, const VkAllocationCallbacks* alloc)
	{
		vkDestroyImageView(device, child, alloc);
	}

	template<>
	inline void destroyVkDeviceChild<VkSampler>(VkDevice device, VkSampler child, const VkAllocationCallbacks* alloc)
	{
		vkDestroySampler(device, child, alloc);
	}

	template<>
	inline void destroyVkDeviceChild<VkDeviceMemory>(VkDevice device, VkDeviceMemory child, const VkAllocationCallbacks* alloc)
	{
		vkFreeMemory(device, child, alloc);
	}

}