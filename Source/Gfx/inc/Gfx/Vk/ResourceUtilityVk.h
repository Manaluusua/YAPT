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
	

    inline uint32_t getTexelBlockSize(VkFormat format) {
        switch (format) {
        case VK_FORMAT_R8_UNORM:
        case VK_FORMAT_R8_SNORM:
        case VK_FORMAT_R8_UINT:
        case VK_FORMAT_R8_SINT:
            return 1;

        case VK_FORMAT_R8G8_UNORM:
        case VK_FORMAT_R8G8_SNORM:
        case VK_FORMAT_R8G8_UINT:
        case VK_FORMAT_R8G8_SINT:
            return 2;

        case VK_FORMAT_R8G8B8_UNORM:
        case VK_FORMAT_R8G8B8_SNORM:
        case VK_FORMAT_R8G8B8_UINT:
        case VK_FORMAT_R8G8B8_SINT:
        case VK_FORMAT_B8G8R8_UNORM:
        case VK_FORMAT_B8G8R8_SNORM:
            return 3;

        case VK_FORMAT_R8G8B8A8_UNORM:
        case VK_FORMAT_R8G8B8A8_SNORM:
        case VK_FORMAT_R8G8B8A8_SRGB:
        case VK_FORMAT_R8G8B8A8_UINT:
        case VK_FORMAT_R8G8B8A8_SINT:
        case VK_FORMAT_B8G8R8A8_UNORM:
        case VK_FORMAT_B8G8R8A8_SNORM:
        case VK_FORMAT_B8G8R8A8_SRGB:
        case VK_FORMAT_B8G8R8A8_UINT:
        case VK_FORMAT_B8G8R8A8_SINT:
            return 4;

        case VK_FORMAT_R16_UNORM:
        case VK_FORMAT_R16_SNORM:
        case VK_FORMAT_R16_UINT:
        case VK_FORMAT_R16_SINT:
        case VK_FORMAT_R16_SFLOAT:
            return 2;

        case VK_FORMAT_R16G16_UNORM:
        case VK_FORMAT_R16G16_SNORM:
        case VK_FORMAT_R16G16_UINT:
        case VK_FORMAT_R16G16_SINT:
        case VK_FORMAT_R16G16_SFLOAT:
            return 4;

        case VK_FORMAT_R16G16B16A16_UNORM:
        case VK_FORMAT_R16G16B16A16_SNORM:
        case VK_FORMAT_R16G16B16A16_UINT:
        case VK_FORMAT_R16G16B16A16_SINT:
        case VK_FORMAT_R16G16B16A16_SFLOAT:
            return 8;

        case VK_FORMAT_R32_UINT:
        case VK_FORMAT_R32_SINT:
        case VK_FORMAT_R32_SFLOAT:
            return 4;

        case VK_FORMAT_R32G32_UINT:
        case VK_FORMAT_R32G32_SINT:
        case VK_FORMAT_R32G32_SFLOAT:
            return 8;

        case VK_FORMAT_R32G32B32_UINT:
        case VK_FORMAT_R32G32B32_SINT:
        case VK_FORMAT_R32G32B32_SFLOAT:
            return 12;

        case VK_FORMAT_R32G32B32A32_UINT:
        case VK_FORMAT_R32G32B32A32_SINT:
        case VK_FORMAT_R32G32B32A32_SFLOAT:
            return 16;

        case VK_FORMAT_BC1_RGB_UNORM_BLOCK:
        case VK_FORMAT_BC1_RGBA_UNORM_BLOCK:
            return 8;

        case VK_FORMAT_BC2_UNORM_BLOCK:
        case VK_FORMAT_BC3_UNORM_BLOCK:
            return 16;

        case VK_FORMAT_BC4_UNORM_BLOCK:
        case VK_FORMAT_BC4_SNORM_BLOCK:
            return 8;

        case VK_FORMAT_BC5_UNORM_BLOCK:
        case VK_FORMAT_BC5_SNORM_BLOCK:
            return 16;

        case VK_FORMAT_BC6H_UFLOAT_BLOCK:
        case VK_FORMAT_BC6H_SFLOAT_BLOCK:
        case VK_FORMAT_BC7_UNORM_BLOCK:
            return 16;

        default:
            assert(!"unknown format!");
            return 0; // Unknown or unsupported format
        }
    }

}