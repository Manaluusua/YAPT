#pragma once


#include <Renderer/Vk/CommonVk.h>
#include <Renderer/Shared/GfxTypes.h>
#include <Renderer/Vk/RendererVk.h>
namespace YAPT
{
	class DescriptorSetLayoutVk;
	class DescriptorSetVk
	{
	public:

		DescriptorSetVk(RendererVk* r, VkDescriptorSet set);
		~DescriptorSetVk();

		void setInUse(bool inUse)
		{
			m_inUse = inUse;
			if (!inUse)
			{
				m_frameLastUsed = m_renderer->getFrameNumber();
			}
		}

		bool isInUse()
		{
			if (m_inUse) return false;
			return (m_frameLastUsed + m_renderer->getFramePipelineLength()) < m_renderer->getFrameNumber();
		}

	private:

		RendererVk* m_renderer;
		VkDescriptorSet m_set;
		size_t m_frameLastUsed;
		bool m_inUse;
	};

}
