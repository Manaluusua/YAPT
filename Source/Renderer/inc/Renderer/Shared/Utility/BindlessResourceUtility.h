#pragma once
#include <Gfx/GfxApi.h>
#include <atomic>
namespace YAPT
{
	class BindlessResourceUtility
	{
	public:
		BindlessResourceUtility();
		~BindlessResourceUtility();

		void init(GfxApiHandle handle, uint32_t numberOfEntries, const DescriptorSetLayoutBinding& bindingDefinition);
		void deinit();

		template<typename ViewHandle>
		void updateDescriptor(uint32_t index, ViewHandle handle);
		

		void flush();

		DescriptorSetLayoutHandle getLayout() const { return m_layout; }
		const DescriptorSetLayoutBinding& getBindingDefinition() const { return m_bindingDef; }
		DescriptorSetHandle getResourceArrayDescSet() const { return m_descriptorSet; }
	private:

		

		GfxApiHandle m_gfxHandle;
		DescriptorSetLayoutHandle m_layout;
		DescriptorSetPoolHandle m_pool;
		DescriptorSetHandle m_descriptorSet;
		DescriptorSetLayoutBinding m_bindingDef;
		std::vector<DescriptorSetUpdate> m_pendingUpdates;
		std::vector<void*> m_handles;
		std::atomic<uint32_t> m_numberOfPendingUpdates;
	};


	template<typename ViewHandle>
	void BindlessResourceUtility::updateDescriptor(uint32_t index, ViewHandle handle)
	{
		uint32_t currentIndex = m_numberOfPendingUpdates.fetch_add(1);
		assert(currentIndex < m_pendingUpdates.size());
		m_handles[currentIndex] = handle;

		DescriptorSetUpdate update;
		update.descriptor = DescriptorPtr((ViewHandle*)& m_handles[currentIndex]);
		update.descriptorCount = 1;
		update.dstArrayElement = index;
		update.dstBinding = m_bindingDef.bindingIndex;
		m_pendingUpdates[currentIndex] = std::move(update);
	}
} 