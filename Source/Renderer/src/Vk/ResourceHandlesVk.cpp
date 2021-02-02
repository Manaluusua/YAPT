#pragma once

#include <Renderer/Vk/ResourceHandlesVk.h>
#include <Renderer/Vk/ResourceManagerVk.h>
#include <Renderer/Vk/YaptToVkConversions.h>
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
		return view;

	}

	void BufferHandleVk::destroyView(BufferViewVk* v)
	{
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
		info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
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