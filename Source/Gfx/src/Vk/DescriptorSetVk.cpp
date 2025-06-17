
#include <Gfx/Vk/DescriptorSetVk.h>
#include <Gfx/Vk/DescriptorSetLayoutVk.h>
#include <Gfx/Vk/ResourceManagerVk.h>
#include <Gfx/Vk/YaptToVkConversions.h>
#include <Gfx/Vk/ResourceHandlesVk.h>
#include <Gfx/Vk/AccelerationStructuresVk.h>

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

		size_t updateWritesCount = 0;
		size_t accStructWritesCount = 0;

		size_t bufferViews = 0;
		size_t imageViews = 0;
		size_t texelBufferViews = 0;
		size_t accelerationStructures = 0;

		for (size_t i = 0; i < updateCount; ++i)
		{
			const DescriptorSetUpdate& descSetSrc = updates[i];
			VkDescriptorType descriptorType = yaptDescriptorTypeToVk(m_layout->getDescriptorLayoutBinding(descSetSrc.dstBinding).type);
			switch (descriptorType)
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
				bufferViews += descSetSrc.descriptorCount;
				break;
			case VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR:
				accelerationStructures += descSetSrc.descriptorCount;
				++accStructWritesCount;
				break;
			}

			++updateWritesCount;
		}

		m_descSetWritesCache.resize(updateWritesCount);
		m_accStructWriteCache.resize(accStructWritesCount);
		m_descriptorBufferInfoScratch.resize(bufferViews);
		m_descriptoImageInfoScratch.resize(imageViews);
		m_descriptorTexelBufferScratch.resize(texelBufferViews);
		m_accelerationStructureRefScratch.resize(accelerationStructures);

		size_t updatesWritten = 0;
		size_t accStructUpdatesWritten = 0;
		size_t bufferDescIndex = 0;
		size_t imageDescIndex = 0;
		size_t texelBufferDescIndex = 0;
		size_t accStructDescIndex = 0;

		for (size_t i = 0; i < updateCount; ++i)
		{
			const DescriptorSetUpdate& descSetSrc = updates[i];
			if (!m_layout->hasBindingAtIndex(descSetSrc.dstBinding)) continue; //skip if the entry is not found from layout (likely been optimized out)
			VkDescriptorType descriptorType = yaptDescriptorTypeToVk(m_layout->getDescriptorLayoutBinding(descSetSrc.dstBinding).type);

			VkWriteDescriptorSet& descSetWrite = m_descSetWritesCache[updatesWritten++];
			descSetWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descSetWrite.pNext = NULL;
			descSetWrite.descriptorCount = descSetSrc.descriptorCount;
			descSetWrite.dstArrayElement = descSetSrc.dstArrayElement;
			descSetWrite.dstBinding = descSetSrc.dstBinding;
			descSetWrite.dstSet = m_set;
			descSetWrite.descriptorType = descriptorType;

			if (descriptorType == VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR)
			{
				VkWriteDescriptorSetAccelerationStructureKHR& accStructWrite = m_accStructWriteCache[accStructUpdatesWritten++];
				accStructWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR;
				accStructWrite.pNext = nullptr;
				accStructWrite.accelerationStructureCount = descSetSrc.descriptorCount;
				accStructWrite.pAccelerationStructures = m_accelerationStructureRefScratch.data() + accStructDescIndex;

				for (size_t k = 0; k < descSetSrc.descriptorCount; ++k)
				{
					m_accelerationStructureRefScratch[accStructDescIndex++] = descSetSrc.descriptor.asAccelerationStructurePtr()[k]->GetAccelerationStructure();
				}
				descSetWrite.pNext = &accStructWrite;
			}
			else
			{
				switch (descSetWrite.descriptorType)
				{
				case VK_DESCRIPTOR_TYPE_SAMPLER:
					descSetWrite.pImageInfo = &m_descriptoImageInfoScratch[imageDescIndex];
					for (uint32_t k = 0; k < descSetSrc.descriptorCount; ++k)
					{
						VkDescriptorImageInfo info;
						info.sampler = descSetSrc.descriptor.asSamplerPtr()[k];
						m_descriptoImageInfoScratch[imageDescIndex++] = info;
					}
					break;
				case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
					/*descSetWrite.pImageInfo = &m_descriptoImageInfoScratch[imageDescIndex];
					for (uint32_t k = 0; k < descSetSrc.descriptorCount; ++k)
					{
						VkDescriptorImageInfo info;
						info.sampler = descSetSrc.samplerHandles[k];
						info.imageView = descSetSrc.texHandles[k];
						info.imageLayout = vkDescriptorTypeToVkImageLayout(descSetWrite.descriptorType);
						m_descriptoImageInfoScratch[imageDescIndex++] = info;
					}*/
					assert(!"not supported!");
					break;
				case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
				case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
				case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:
					descSetWrite.pImageInfo = &m_descriptoImageInfoScratch[imageDescIndex];
					for (uint32_t k = 0; k < descSetSrc.descriptorCount; ++k)
					{
						VkDescriptorImageInfo info;
						info.imageView = descSetSrc.descriptor.asTextureViewPtr()[k];
						info.imageLayout = vkDescriptorTypeToVkImageLayout(descSetWrite.descriptorType);
						m_descriptoImageInfoScratch[imageDescIndex++] = info;
					}
					break;
				case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
				case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
					descSetWrite.pTexelBufferView = &m_descriptorTexelBufferScratch[texelBufferDescIndex];
					for (uint32_t k = 0; k < descSetSrc.descriptorCount; ++k)
					{
						m_descriptorTexelBufferScratch[texelBufferDescIndex++] = descSetSrc.descriptor.asBufferViewPtr()[k]->bufferView;
					}
					break;
				case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
				case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
				case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
				case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
				case VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK_EXT:
				{
					descSetWrite.pBufferInfo = &m_descriptorBufferInfoScratch[bufferDescIndex];
					bool isDynamic = descSetWrite.descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC || descSetWrite.descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC ? true : false;
					for (uint32_t k = 0; k < descSetSrc.descriptorCount; ++k)
					{
						auto bufferViewPtr = descSetSrc.descriptor.asBufferViewPtr()[k];
						bool raw = (bufferViewPtr->bufferDesc.flags & BufferViewFlagBits::BUFFERVIEWFLAGS_RAW) != 0;
						size_t stride = raw ? 4 : bufferViewPtr->bufferDesc.structureStrideInBytes;
						VkDescriptorBufferInfo info;
						info.buffer = bufferViewPtr->buffer;
						info.offset = bufferViewPtr->bufferDesc.offsetInBytes;
						info.range = isDynamic ? stride : bufferViewPtr->bufferDesc.sizeInBytes; //dynamic buffers are only bound by stride and offset when binding descset
						m_descriptorBufferInfoScratch[bufferDescIndex++] = info;
					}
				}
					
				break;
				default:
					assert(!"unsupported descriptor type");
					break;
				}
			}
		}


		vkUpdateDescriptorSets(m_renderer->getDevice(), (uint32_t)updatesWritten, m_descSetWritesCache.data(), 0, NULL);
	}

}