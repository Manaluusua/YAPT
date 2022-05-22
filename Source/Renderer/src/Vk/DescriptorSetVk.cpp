
#include <Renderer/Vk/DescriptorSetVk.h>
#include <Renderer/Vk/DescriptorSetLayoutVk.h>
#include <Renderer/Vk/ResourceManagerVk.h>

namespace YAPT
{
	DescriptorSetVk::DescriptorSetVk(RendererVk* r, VkDescriptorSet set)
		:m_renderer(r),
		m_set(set),
		m_inUse(false),
		m_frameLastUsed(uint32_t(-1))
	{

	}
	DescriptorSetVk::~DescriptorSetVk()
	{

	}
}