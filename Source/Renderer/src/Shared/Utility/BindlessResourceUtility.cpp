#include <Renderer/Shared/Utility/BindlessResourceUtility.h>


namespace YAPT
{
	BindlessResourceUtility::BindlessResourceUtility()
		:m_gfxHandle(YAPT_NULL_HANDLE),
		m_layout(YAPT_NULL_HANDLE),
		m_pool(YAPT_NULL_HANDLE),
		m_descriptorSet(YAPT_NULL_HANDLE)
	{

	}
	BindlessResourceUtility::~BindlessResourceUtility()
	{

	}

	void BindlessResourceUtility::init(GfxApiHandle handle, uint32_t numberOfEntries, const DescriptorSetLayoutBinding& bindingDefinition)
	{
		m_gfxHandle = handle;

		m_bindingDef = bindingDefinition;
		m_layout = Gfx::createDescriptorSetLayout(m_gfxHandle, &m_bindingDef, 1, DESCRIPTORSETLAYOUTFLAG_USE_BINDING_POINT_ALIASING);
		m_pool = Gfx::createDescriptorSetPool(m_gfxHandle, m_layout, 1);
		m_descriptorSet = Gfx::getDescriptorSet(m_pool, 0);
		
	}

	void BindlessResourceUtility::deinit()
	{
		if (m_layout != YAPT_NULL_HANDLE)
		{
			Gfx::destroyDescriptorSetPool(m_gfxHandle, m_pool);
			m_pool = YAPT_NULL_HANDLE;
			m_descriptorSet = YAPT_NULL_HANDLE;

			Gfx::destroyDescriptorSetLayout(m_gfxHandle, m_layout);
			m_layout = YAPT_NULL_HANDLE;
		}
	}
	
	void BindlessResourceUtility::updateDescriptor(uint32_t index, TextureViewHandle handle)
	{
		TextureViewHandle h = handle;

		DescriptorSetUpdate update;
		update.texHandles = &h;
		update.descriptorCount = 1;
		update.dstArrayElement = index;
		update.dstBinding = m_bindingDef.bindingIndex;
		Gfx::updateDescriptorSet(m_gfxHandle, m_descriptorSet, &update, 1);
	}
	void BindlessResourceUtility::updateDescriptor(uint32_t index, BufferViewHandle handle)
	{
		BufferViewHandle h = handle;

		DescriptorSetUpdate update;
		update.buffHandles = &h;
		update.descriptorCount = 1;
		update.dstArrayElement = index;
		update.dstBinding = m_bindingDef.bindingIndex;
		 
		Gfx::updateDescriptorSet(m_gfxHandle, m_descriptorSet, &update, 1);
	}
	 
}