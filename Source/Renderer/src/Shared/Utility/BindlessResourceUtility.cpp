#include <Renderer/Shared/Utility/BindlessResourceUtility.h>


namespace YAPT
{
	BindlessResourceUtility::BindlessResourceUtility()
		:m_gfxHandle(YAPT_NULL_HANDLE),
		m_layout(YAPT_NULL_HANDLE),
		m_pool(YAPT_NULL_HANDLE),
		m_descriptorSet(YAPT_NULL_HANDLE),
		m_numberOfPendingUpdates(0)
	{

	}
	BindlessResourceUtility::~BindlessResourceUtility()
	{

	}

	void BindlessResourceUtility::init(GfxApiHandle handle, uint32_t numberOfEntries, const DescriptorSetLayoutBinding& bindingDefinition)
	{
		m_gfxHandle = handle;

		m_bindingDef = bindingDefinition;
		m_layout = Gfx::createDescriptorSetLayout(m_gfxHandle, &m_bindingDef, 1, DESCRIPTORSETLAYOUTFLAG_BINDINGS_MAY_ALIAS);
		m_pool = Gfx::createDescriptorSetPool(m_gfxHandle, m_layout, 1);
		m_descriptorSet = Gfx::getDescriptorSet(m_pool, 0);
		m_numberOfPendingUpdates = 0;
		m_pendingUpdates.resize(numberOfEntries);
		m_handles.resize(numberOfEntries);
	}

	void BindlessResourceUtility::flush()
	{
		Gfx::updateDescriptorSet(m_gfxHandle, m_descriptorSet, m_pendingUpdates.data(), m_numberOfPendingUpdates);
		m_numberOfPendingUpdates = 0;
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
}