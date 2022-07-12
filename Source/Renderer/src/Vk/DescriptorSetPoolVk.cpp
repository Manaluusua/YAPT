#include <Renderer/Vk/DescriptorSetPoolVk.h>
#include <Renderer/Vk/DescriptorSetLayoutVk.h>
#include <Renderer/Vk/RendererVk.h>
#include <Renderer/Vk/ResourceManagerVk.h>
namespace YAPT
{
	DescriptorSetPoolVk* DescriptorSetPoolVk::create(RendererVk* renderer, DescriptorSetLayoutVk* layout, size_t numberOfDescriptorSets)
	{
		VkDescriptorPoolCreateInfo poolDef;
		poolDef.pNext = nullptr;
		poolDef.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolDef.maxSets = (uint32_t)numberOfDescriptorSets;
		poolDef.flags = (layout->getFlags() & DESCRIPTORSETLAYOUTFLAG_BINDINGS_MAY_ALIAS) != 0 ? VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT : 0;

		poolDef.pPoolSizes = layout->getDescriptorCountPerType();
		poolDef.poolSizeCount = (uint32_t)layout->getDescriptorTypesCount();
		VkDescriptorPool pool;
		VkResult res = vkCreateDescriptorPool(renderer->getDevice(), &poolDef, VK_ALLOC_CB, &pool);

		assert(res == VK_SUCCESS);
		if (res != VK_SUCCESS) return nullptr;

		
		std::vector<VkDescriptorSet> descSets;
		descSets.resize(numberOfDescriptorSets);
		std::vector<VkDescriptorSetLayout> layoutArray;
		layoutArray.resize(numberOfDescriptorSets, layout->getLayout());

		VkDescriptorSetAllocateInfo allocInfo;
		allocInfo.pNext = nullptr;
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorSetCount = (uint32_t)numberOfDescriptorSets;
		allocInfo.descriptorPool = pool;
		allocInfo.pSetLayouts = layoutArray.data();

		res = vkAllocateDescriptorSets(renderer->getDevice(), &allocInfo, descSets.data());

		assert(res == VK_SUCCESS);
		if (res != VK_SUCCESS) return nullptr;


		DescriptorSetPoolVk* p = new DescriptorSetPoolVk(renderer, pool, layout, numberOfDescriptorSets, descSets.data());
		return p;
	}

	DescriptorSetPoolVk::DescriptorSetPoolVk(RendererVk* renderer, VkDescriptorPool pool, DescriptorSetLayoutVk* layout, size_t numberOfSets, VkDescriptorSet* sets)
		:m_renderer(renderer),
		m_pool(pool)
	{
		
		m_sets.reserve(numberOfSets);
		for (size_t i = 0; i < numberOfSets; ++i)
		{
			m_sets.emplace_back(renderer, sets[i], layout);
		}
		
	}

	DescriptorSetPoolVk::~DescriptorSetPoolVk()
	{
		m_renderer->getResourceManager()->deferredDestroyVkResource(m_pool);
	}
}

