#pragma once

#include <Renderer/Vk/CommonVk.h>
#include <Renderer/Shared/GfxTypes.h>
#include <Renderer/Vk/DescriptorSetVk.h>
#include <assert.h>
namespace YAPT
{
	class RendererVk;
	class DescriptorSetLayoutVk;
	class DescriptorSetPoolVk
	{
	public:

		static DescriptorSetPoolVk* create(RendererVk* renderer, DescriptorSetLayoutVk* layout, size_t numberOfDescriptorSets);

		DescriptorSetVk* getDescriptorSet(size_t setIndex)
		{
			assert(setIndex < m_sets.size());
			if (setIndex >= m_sets.size()) return nullptr;
			return &m_sets[setIndex];
		}
		~DescriptorSetPoolVk();

	private:

		DescriptorSetPoolVk(RendererVk* renderer, VkDescriptorPool pool, DescriptorSetLayoutVk* layout, size_t numberOfSets, VkDescriptorSet* sets);
		RendererVk* m_renderer;
		VkDescriptorPool m_pool;
		std::vector<DescriptorSetVk> m_sets;
	};
}
