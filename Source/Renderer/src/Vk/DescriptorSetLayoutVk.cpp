#include <Renderer/Vk/DescriptorSetLayoutVk.h>
#include <Renderer/Vk/YaptToVkConversions.h>
#include <Renderer/Vk/RendererVk.h>
#include <Renderer/Vk/ResourceManagerVk.h>
#include <unordered_map>
namespace YAPT
{

	DescriptorSetLayoutVk* DescriptorSetLayoutVk::create(RendererVk* renderer, const DescriptorSetLayoutBinding* bindings, size_t numberOfBindings, DescriptorSetLayoutFlags flags)
	{
		VkDescriptorSetLayout layout = VK_NULL_HANDLE;
		VkDescriptorSetLayoutCreateInfo info;
		std::vector<VkDescriptorSetLayoutBinding> vkBindings;

		std::unordered_map<VkDescriptorType, uint32_t> countPerType;

		vkBindings.resize(numberOfBindings);
		info.pNext = nullptr;
		info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		info.flags = (flags & DESCRIPTORSETLAYOUTFLAG_BINDINGS_MAY_ALIAS) != 0 ? VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT : 0;
		info.bindingCount = (uint32_t)vkBindings.size();
		info.pBindings = vkBindings.data();

		for (size_t i = 0; i < numberOfBindings; ++i)
		{
			VkDescriptorSetLayoutBinding& vkBinding = vkBindings[i];
			const DescriptorSetLayoutBinding& binding = bindings[i];

			vkBinding.binding = binding.bindingIndex;
			vkBinding.descriptorCount = binding.descriptorCount;
			vkBinding.stageFlags = yaptShaderStagesToVk(binding.shaderStages);
			vkBinding.descriptorType = yaptDescriptorTypeToVk(binding.type);
			vkBinding.pImmutableSamplers = binding.staticSamplers;

			countPerType[vkBinding.descriptorType] += vkBinding.descriptorCount;
		}


		VkResult res = vkCreateDescriptorSetLayout(renderer->getDevice(), &info, VK_ALLOC_CB, &layout);

		if (res != VK_SUCCESS)
		{
			YAPT_LOG_FATAL_ERROR("Vk Error (%d)", res);
			return nullptr;
		}

		DescriptorSetLayoutVk* handle = new DescriptorSetLayoutVk;
		handle->m_renderer = renderer;
		handle->m_layout = layout;
		for (auto iter = countPerType.begin(); iter != countPerType.end(); ++iter)
		{
			handle->m_requiredDescriptorSpacePerType.push_back({ iter->first, iter->second });
		}
		handle->m_flags = flags;
		return handle;
	}

	DescriptorSetLayoutVk::DescriptorSetLayoutVk()
	{
		
	}
	DescriptorSetLayoutVk::~DescriptorSetLayoutVk()
	{
		m_renderer->getResourceManager()->deferredDestroyVkResource(m_layout);
	}
}