#pragma once
#include <Gfx/Utility/ResourceViewPool.h>
#include <Gfx/Vk/CommonVk.h>
#include <Gfx/GfxTypes.h>
#include <Gfx/Utility/ResourceStateTracker.h>
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
		uint32_t owningQueueFamily;
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
		uint32_t owningQueueFamily;
		ResourceStateTracker<VkImageLayout> currentLayouts;
		char* mappedMemory;
#ifdef VK_DEBUGNAMES_ENABLE
		std::string name;
#endif
	};

}