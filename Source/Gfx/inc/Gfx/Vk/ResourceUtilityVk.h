#pragma once

#include <Gfx/Vk/CommonVk.h>

namespace YAPT
{
	template<typename T>
	inline void destroyVkDeviceChild(VkDevice device, const VkExtensions& extensions, T child, const VkAllocationCallbacks* alloc)
	{
		dummyDestroyVkNotImplemented(device, child, alloc);
	}

	template<>
	inline void destroyVkDeviceChild<VkBuffer>(VkDevice device, const VkExtensions& extensions, VkBuffer child, const VkAllocationCallbacks* alloc)
	{
		vkDestroyBuffer(device, child, alloc);
	}

	template<>
	inline void destroyVkDeviceChild<VkImage>(VkDevice device, const VkExtensions& extensions, VkImage child, const VkAllocationCallbacks* alloc)
	{
		vkDestroyImage(device, child, alloc);
	}

	template<>
	inline void destroyVkDeviceChild<VkImageView>(VkDevice device, const VkExtensions& extensions, VkImageView child, const VkAllocationCallbacks* alloc)
	{
		vkDestroyImageView(device, child, alloc);
	}

	template<>
	inline void destroyVkDeviceChild<VkSampler>(VkDevice device, const VkExtensions& extensions, VkSampler child, const VkAllocationCallbacks* alloc)
	{
		vkDestroySampler(device, child, alloc);
	}

	template<>
	inline void destroyVkDeviceChild<VkDeviceMemory>(VkDevice device, const VkExtensions& extensions, VkDeviceMemory child, const VkAllocationCallbacks* alloc)
	{
		vkFreeMemory(device, child, alloc);
	}

	template<>
	inline void destroyVkDeviceChild<VkDescriptorSetLayout>(VkDevice device, const VkExtensions& extensions, VkDescriptorSetLayout child, const VkAllocationCallbacks* alloc)
	{
		vkDestroyDescriptorSetLayout(device, child, VK_ALLOC_CB);
	}

	template<>
	inline void destroyVkDeviceChild<VkDescriptorPool>(VkDevice device, const VkExtensions& extensions, VkDescriptorPool child, const VkAllocationCallbacks* alloc)
	{
		vkDestroyDescriptorPool(device, child, VK_ALLOC_CB);
	}

	template<>
	inline void destroyVkDeviceChild<VkPipeline>(VkDevice device, const VkExtensions& extensions, VkPipeline child, const VkAllocationCallbacks* alloc)
	{
		vkDestroyPipeline(device, child, VK_ALLOC_CB);
	}

	template<>
	inline void destroyVkDeviceChild<VkBufferView>(VkDevice device, const VkExtensions& extensions, VkBufferView child, const VkAllocationCallbacks* alloc)
	{
		vkDestroyBufferView(device, child, VK_ALLOC_CB);
	}

	template<>
	inline void destroyVkDeviceChild<VkFramebuffer>(VkDevice device, const VkExtensions& extensions, VkFramebuffer child, const VkAllocationCallbacks* alloc)
	{
		vkDestroyFramebuffer(device, child, VK_ALLOC_CB);
	}

	template<>
	inline void destroyVkDeviceChild<VkShaderModule>(VkDevice device, const VkExtensions& extensions, VkShaderModule child, const VkAllocationCallbacks* alloc)
	{
		vkDestroyShaderModule(device, child, VK_ALLOC_CB);
	}

	template<>
	inline void destroyVkDeviceChild<VkRenderPass>(VkDevice device, const VkExtensions& extensions, VkRenderPass child, const VkAllocationCallbacks* alloc)
	{
		vkDestroyRenderPass(device, child, VK_ALLOC_CB);
	}

	template<>
	inline void destroyVkDeviceChild<VkAccelerationStructureKHR>(VkDevice device, const VkExtensions& extensions, VkAccelerationStructureKHR child, const VkAllocationCallbacks* alloc)
	{
		extensions.vkDestroyAccelerationStructureKHR(device, child, VK_ALLOC_CB);
	}
	
}