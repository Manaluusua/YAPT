#pragma once
#include <Renderer/Shared/Utility/ResourceViewPool.h>
#include <Renderer/Vk/CommonVk.h>
#include <Renderer/Shared/GfxTypes.h>
#include <Renderer/Shared/Utility/ResourceStateTracker.h>
namespace YAPT
{
	class ResourceManagerVk;

	struct BufferViewVk
	{
		BufferViewDesc bufferDesc;
		VkBuffer buffer;
		VkBufferView bufferView;
	};

	struct BufferHandleVk
	{
		typedef ResourceViewPool<BufferViewDesc, BufferViewVk*, BufferHandleVk, 3> BufferViews;
		BufferHandleVk(ResourceManagerVk& mngr);
		BufferViewVk* createView(const BufferViewDesc& v);
		void destroyView(BufferViewVk* v);

		ResourceManagerVk& resMngr;
		BufferViews views;
		VkBuffer buffer;
		VkBufferCreateInfo createInfo;
		VkMemoryPropertyFlags memoryFlags;
		char* mappedMemory;
#ifdef VK_DEBUGNAMES_ENABLE
		std::string name;
#endif
	};

	struct TextureHandleVk
	{
		typedef ResourceViewPool<TextureViewDesc, VkImageView, TextureHandleVk, 3> TextureViews;
		TextureHandleVk(ResourceManagerVk& mngr);
		VkImageView createView(const TextureViewDesc& texView);
		void destroyView(VkImageView texView);

		ResourceManagerVk& resMngr;
		TextureViews views;
		VkImage image;
		ResourceDimension dimensions;
		VkImageCreateInfo createInfo;
		VkMemoryPropertyFlags memoryFlags;
		ResourceStateTracker<VkImageLayout> currentLayouts;
		char* mappedMemory;
#ifdef VK_DEBUGNAMES_ENABLE
		std::string name;
#endif
	};

}