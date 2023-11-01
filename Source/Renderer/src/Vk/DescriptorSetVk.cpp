
#include <Renderer/Vk/DescriptorSetVk.h>
#include <Renderer/Vk/DescriptorSetLayoutVk.h>
#include <Renderer/Vk/ResourceManagerVk.h>
#include <Renderer/Vk/YaptToVkConversions.h>
#include <Renderer/Vk/ResourceHandlesVk.h>

namespace YAPT
{
	DescriptorSetVk::DescriptorSetVk(RendererVk* r, VkDescriptorSet set, DescriptorSetLayoutVk* layout)
		:m_renderer(r),
		m_set(set),
		m_layout(layout),
		m_inUse(false),
		m_frameLastUsed(uint32_t(-1))
	{

	}
	DescriptorSetVk::~DescriptorSetVk()
	{

	}

	void DescriptorSetVk::updateContents(const DescriptorSetUpdate* updates, size_t updateCount)
	{

		std::vector<VkWriteDescriptorSet> descSetWrites;
		descSetWrites.resize(updateCount);

		size_t bufferViews = 0;
		size_t imageViews = 0;
		size_t texelBufferViews = 0;

		for (size_t i = 0; i < updateCount; ++i)
		{

			VkWriteDescriptorSet& descSetWrite = descSetWrites[i];
			const DescriptorSetUpdate& descSetSrc = updates[i];

			descSetWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descSetWrite.pNext = NULL;

			descSetWrite.descriptorCount = descSetSrc.descriptorCount;
			descSetWrite.dstArrayElement = descSetSrc.dstArrayElement;
			descSetWrite.dstBinding = descSetSrc.dstBinding;
			descSetWrite.dstSet = m_set;

			assert(m_layout->hasBindingAtIndex(descSetSrc.dstBinding));
			descSetWrite.descriptorType = yaptDescriptorTypeToVk(m_layout->getDescriptorLayoutBinding(descSetSrc.dstBinding).type);

			switch (descSetWrite.descriptorType)
			{
			case VK_DESCRIPTOR_TYPE_SAMPLER:
			case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
			case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
			case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
			case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:
				imageViews += descSetSrc.descriptorCount;
				break;
			case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
			case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
				texelBufferViews += descSetSrc.descriptorCount;
				break;
			case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
			case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
			case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
			case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
			case VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK_EXT:
			case VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR:
			case VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_NV:
				bufferViews += descSetSrc.descriptorCount;
				break;

			}
		}

		m_descriptorBufferInfoScratch.resize(bufferViews);
		m_descriptoImageInfoScratch.resize(imageViews);
		m_descriptorTexelBufferScratch.resize(texelBufferViews);


		size_t bufferDescIndex = 0;
		size_t imageDescIndex = 0;
		size_t texelBufferDescIndex = 0;

		for (size_t i = 0; i < updateCount; ++i)
		{
			VkWriteDescriptorSet& descSetWrite = descSetWrites[i];
			const DescriptorSetUpdate& descSetSrc = updates[i];

			switch (descSetWrite.descriptorType)
			{
			case VK_DESCRIPTOR_TYPE_SAMPLER:
				descSetWrite.pImageInfo = &m_descriptoImageInfoScratch[imageDescIndex];
				for (uint32_t k = 0; k < descSetSrc.descriptorCount; ++k)
				{
					VkDescriptorImageInfo info;
					info.sampler = descSetSrc.samplerHandles[k];
					m_descriptoImageInfoScratch[imageDescIndex++] = info;
				}
				break;
			case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
				descSetWrite.pImageInfo = &m_descriptoImageInfoScratch[imageDescIndex];
				for (uint32_t k = 0; k < descSetSrc.descriptorCount; ++k)
				{
					VkDescriptorImageInfo info;
					info.sampler = descSetSrc.samplerHandles[k];
					info.imageView = descSetSrc.texHandles[k];
					info.imageLayout = vkDescriptorTypeToVkImageLayout(descSetWrite.descriptorType);
					m_descriptoImageInfoScratch[imageDescIndex++] = info;
				}
				break;
			case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
			case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
			case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:
				descSetWrite.pImageInfo = &m_descriptoImageInfoScratch[imageDescIndex];
				for (uint32_t k = 0; k < descSetSrc.descriptorCount; ++k)
				{
					VkDescriptorImageInfo info;
					info.imageView = descSetSrc.texHandles[k];
					info.imageLayout = vkDescriptorTypeToVkImageLayout(descSetWrite.descriptorType);
					m_descriptoImageInfoScratch[imageDescIndex++] = info;
				}
				break;
			case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
			case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
				descSetWrite.pTexelBufferView = &m_descriptorTexelBufferScratch[texelBufferDescIndex];
				for (uint32_t k = 0; k < descSetSrc.descriptorCount; ++k)
				{
					m_descriptorTexelBufferScratch[texelBufferDescIndex++] = descSetSrc.buffHandles[k]->bufferView;
				}
				break;
			case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
			case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
			case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
			case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
			case VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK_EXT:
			case VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR:
			case VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_NV:
				descSetWrite.pBufferInfo = &m_descriptorBufferInfoScratch[bufferDescIndex];
				bool isDynamic = descSetWrite.descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC || descSetWrite.descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC ? true : false;
				for (uint32_t k = 0; k < descSetSrc.descriptorCount; ++k)
				{
					VkDescriptorBufferInfo info;
					info.buffer = descSetSrc.buffHandles[k]->buffer;
					info.offset = descSetSrc.buffHandles[k]->bufferDesc.offsetInBytes;
					info.range = isDynamic ? descSetSrc.buffHandles[k]->bufferDesc.structureStrideInBytes : descSetSrc.buffHandles[k]->bufferDesc.sizeInBytes; //dynamic buffers are only bound by stride and offset when binding descset
					m_descriptorBufferInfoScratch[bufferDescIndex++] = info;
				}
				break;

			}
		}


		vkUpdateDescriptorSets(m_renderer->getDevice(), (uint32_t)updateCount, descSetWrites.data(), 0, NULL);
	}

}