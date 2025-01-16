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

		DescriptorSetVk(RendererVk* r, VkDescriptorSet set, DescriptorSetLayoutVk* layout);
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

		void updateContents(const DescriptorSetUpdate* updates, size_t updateCount);

		VkDescriptorSet nativeDescriptorSet() { return m_set; }

	private:
		RendererVk* m_renderer;
		VkDescriptorSet m_set;
		DescriptorSetLayoutVk* m_layout;

		std::vector<VkWriteDescriptorSet> m_descSetWritesCache;
		std::vector<VkWriteDescriptorSetAccelerationStructureKHR> m_accStructWriteCache;
		std::vector<VkDescriptorBufferInfo> m_descriptorBufferInfoScratch;
		std::vector<VkDescriptorImageInfo> m_descriptoImageInfoScratch;
		std::vector<VkBufferView> m_descriptorTexelBufferScratch;
		std::vector<VkAccelerationStructureKHR> m_accelerationStructureRefScratch;

		size_t m_frameLastUsed;
		bool m_inUse;
	};

}
