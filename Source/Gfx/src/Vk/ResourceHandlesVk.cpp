#pragma once

#include <Gfx/Vk/ResourceHandlesVk.h>
#include <Gfx/Vk/ResourceManagerVk.h>
#include <Gfx/Vk/YaptToVkConversions.h>
namespace YAPT
{
	BufferHandleVk::BufferHandleVk(ResourceManagerVk& mngr)
		:resMngr(mngr),
		views(*this)
	{

	}


	BufferViewVk* BufferHandleVk::createView(const BufferViewDesc& v)
	{
		BufferViewVk* view = new BufferViewVk;
		view->bufferDesc = v;
		view->buffer = buffer;
		view->bufferView = VK_NULL_HANDLE;

		if (v.nonStructuredFormat != ResourceFormat::UNKNOWN && ((createInfo.usage & (VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER | VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER) ) != 0) )
		{
			VkBufferViewCreateInfo info;
			info.sType = VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO;
			info.pNext = nullptr;
			info.offset = v.offsetInBytes;
			info.range = v.sizeInBytes;
			info.buffer = buffer;
			info.flags = 0;
			info.format = yaptFormatToVk(v.nonStructuredFormat);
			VkResult res = vkCreateBufferView(resMngr.getDevice(), &info, VK_ALLOC_CB, &view->bufferView);
			checkForVkError(res);
		}

		return view;

	}

	void BufferHandleVk::destroyView(BufferViewVk* v)
	{
		if (v->bufferView != VK_NULL_HANDLE)
		{
			resMngr.deferredDestroyVkResource(v->bufferView);
		}
		delete v;
	}

	//////////////////////////////////////////////////////////////////////////////////

	TextureHandleVk::TextureHandleVk(ResourceManagerVk& mngr)
		:resMngr(mngr),
		views(*this)
	{

	}

	VkImageView TextureHandleVk::createView(const TextureViewDesc& texView)
	{
		VkImageView v;
		VkImageViewCreateInfo info{};
		info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		info.viewType = yaptResourceDimensionToVkViewType(texView.dimensions);
		info.image = image;
		info.format = yaptFormatToVk(texView.format);
		info.subresourceRange.baseArrayLayer = texView.arraySliceOffset;
		info.subresourceRange.layerCount = texView.arraySliceCount;
		info.subresourceRange.baseMipLevel = texView.mipOffset;
		info.subresourceRange.levelCount = texView.mipCount;

		info.flags = 0;
		info.subresourceRange.aspectMask = texView.resourceUsage == RESOURCE_USAGE_UNKNOWN ? VK_IMAGE_ASPECT_COLOR_BIT : yaptUsageToAspectFlags(texView.resourceUsage);
		info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

		VkResult res = vkCreateImageView(resMngr.getDevice(), &info, VK_ALLOC_CB, &v);
		checkForVkError(res);
		return v;
	}

	void TextureHandleVk::destroyView(VkImageView texView)
	{
		resMngr.deferredDestroyVkResource(texView);
	}


	

}