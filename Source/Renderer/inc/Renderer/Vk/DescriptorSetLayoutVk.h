#pragma once

#include <Renderer/Vk/CommonVk.h>
#include <Renderer/Shared/GfxTypes.h>

namespace YAPT
{
	class RendererVk;
	
	class DescriptorSetLayoutVk
	{
	public:

		static DescriptorSetLayoutVk* create(RendererVk* renderer, const DescriptorSetLayoutBinding* bindings, size_t numberOfBindings, DescriptorSetLayoutFlags flags);

		VkDescriptorSetLayout getLayout() const { return m_layout; }
		DescriptorSetLayoutFlags getFlags() const { return m_flags; }
		const VkDescriptorPoolSize* getDescriptorCountPerType() const { return m_requiredDescriptorSpacePerType.data(); }
		const size_t getDescriptorTypesCount() const { return m_requiredDescriptorSpacePerType.size(); }
		
		~DescriptorSetLayoutVk();

	private:

		DescriptorSetLayoutVk();

		RendererVk* m_renderer;
		VkDescriptorSetLayout m_layout;
		std::vector<VkDescriptorPoolSize> m_requiredDescriptorSpacePerType;
		DescriptorSetLayoutFlags m_flags;
	};
}
