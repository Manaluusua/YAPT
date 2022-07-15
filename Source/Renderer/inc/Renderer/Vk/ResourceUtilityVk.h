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

	template<>
	inline void destroyVkDeviceChild<VkDescriptorSetLayout>(VkDevice device, VkDescriptorSetLayout child, const VkAllocationCallbacks* alloc)
	{
		vkDestroyDescriptorSetLayout(device, child, VK_ALLOC_CB);
	}

	template<>
	inline void destroyVkDeviceChild<VkDescriptorPool>(VkDevice device, VkDescriptorPool child, const VkAllocationCallbacks* alloc)
	{
		vkDestroyDescriptorPool(device, child, VK_ALLOC_CB);
	}

	template<>
	inline void destroyVkDeviceChild<VkPipeline>(VkDevice device, VkPipeline child, const VkAllocationCallbacks* alloc)
	{
		vkDestroyPipeline(device, child, VK_ALLOC_CB);
	}

	template<>
	inline void destroyVkDeviceChild<VkBufferView>(VkDevice device, VkBufferView child, const VkAllocationCallbacks* alloc)
	{
		vkDestroyBufferView(device, child, VK_ALLOC_CB);
	}
	
	
}