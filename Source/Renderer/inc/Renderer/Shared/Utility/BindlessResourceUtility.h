#pragma once
#include <Gfx/GfxApi.h>
namespace YAPT
{
	class BindlessResourceUtility
	{
	public:
		BindlessResourceUtility();
		~BindlessResourceUtility();

		void init(GfxApiHandle handle, uint32_t numberOfEntries, const DescriptorSetLayoutBinding& bindingDefinition);
		void deinit();

		//TODO: maybe gather and batch update calls? for now done individually
		void updateDescriptor(uint32_t index, TextureViewHandle handle);
		void updateDescriptor(uint32_t index, BufferViewHandle handle);

		DescriptorSetLayoutHandle getLayout() const { return m_layout; }
		const DescriptorSetLayoutBinding& getBindingDefinition() const { return m_bindingDef; }
		DescriptorSetHandle getResourceArrayDescSet() const { return m_descriptorSet; }
	private:
		GfxApiHandle m_gfxHandle;
		DescriptorSetLayoutHandle m_layout;
		DescriptorSetPoolHandle m_pool;
		DescriptorSetHandle m_descriptorSet;
		DescriptorSetLayoutBinding m_bindingDef;
	};
} 