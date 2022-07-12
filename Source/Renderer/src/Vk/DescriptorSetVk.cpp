
#include <Renderer/Vk/DescriptorSetVk.h>
#include <Renderer/Vk/DescriptorSetLayoutVk.h>
#include <Renderer/Vk/ResourceManagerVk.h>
#include <Renderer/Vk/YaptToVkConversions.h>

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


			assert(!"TODO actual handles");

		}

		vkUpdateDescriptorSets(m_renderer->getDevice(), (uint32_t)updateCount, descSetWrites.data(), 0, NULL);
	}

}