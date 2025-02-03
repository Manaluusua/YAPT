#include <Gfx/Vk/DescriptorSetLayoutVk.h>
#include <Gfx/Vk/YaptToVkConversions.h>
#include <Gfx/Vk/RendererVk.h>
#include <Gfx/Vk/ResourceManagerVk.h>
#include <unordered_map>
namespace YAPT
{
	static const DescriptorSetLayoutBinding g_dummyBinding = { uint32_t(-1), 0 };

	DescriptorSetLayoutVk* DescriptorSetLayoutVk::create(RendererVk* renderer, const DescriptorSetLayoutBinding* bindings, size_t numberOfBindings, DescriptorSetLayoutFlags flags)
	{
		VkDescriptorSetLayout layout = VK_NULL_HANDLE;
		VkDescriptorSetLayoutCreateInfo info;
		VkDescriptorSetLayoutBindingFlagsCreateInfo extraCreateInfo;
		std::vector<VkDescriptorSetLayoutBinding> vkBindings;
		std::vector<VkDescriptorBindingFlags> extraFlags;

		std::unordered_map<VkDescriptorType, uint32_t> countPerType;

		const VkDescriptorBindingFlagsEXT extraBindingFlags =
			VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT |
			VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT |
			VK_DESCRIPTOR_BINDING_UPDATE_UNUSED_WHILE_PENDING_BIT;
		

		bool updateAfterBind = (flags & DESCRIPTORSETLAYOUTFLAG_BINDINGS_MAY_ALIAS) != 0;

		vkBindings.resize(numberOfBindings);

		info.pNext = nullptr;
		info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		info.flags = updateAfterBind ? VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT : 0;
		info.bindingCount = (uint32_t)vkBindings.size();
		info.pBindings = vkBindings.data();

		if (updateAfterBind)
		{
			extraFlags.resize(numberOfBindings);
			extraCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
			extraCreateInfo.pNext = NULL;
			extraCreateInfo.bindingCount = (uint32_t)numberOfBindings;
			extraCreateInfo.pBindingFlags = extraFlags.data();
			info.pNext = &extraCreateInfo;
		}

		uint32_t largestBindingIndex = 0;

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
			largestBindingIndex = std::max(largestBindingIndex, binding.bindingIndex);
			if (updateAfterBind)
			{
				extraFlags[i] = extraBindingFlags;
			}
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

		

		handle->m_bindings.assign(size_t(largestBindingIndex) + 1u, g_dummyBinding);
		
		for (uint32_t i = 0; i != numberOfBindings; ++i)
		{
			const DescriptorSetLayoutBinding& binding = bindings[i];
			handle->m_bindings[binding.bindingIndex] = binding;
		}

		return handle;
	}

	const DescriptorSetLayoutBinding& DescriptorSetLayoutVk::getDescriptorLayoutBinding(size_t bindingIndex)
	{
		if (bindingIndex >= m_bindings.size()) return g_dummyBinding;

		return m_bindings[bindingIndex];
	}

	bool DescriptorSetLayoutVk::hasBindingAtIndex(size_t bindingIndex)
	{
		return getDescriptorLayoutBinding(bindingIndex).bindingIndex != uint32_t(-1);
	}

	DescriptorSetLayoutVk::DescriptorSetLayoutVk()
	{
		
	}
	DescriptorSetLayoutVk::~DescriptorSetLayoutVk()
	{
		m_renderer->getResourceManager()->deferredDestroyVkResource(m_layout);
	}
}