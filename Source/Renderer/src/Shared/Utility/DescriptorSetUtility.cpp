#include <Renderer/Shared/Utility/DescriptorSetUtility.h>

#include <Renderer/Shared/GfxApi.h>
#include <Renderer/Shared/CRenderer.h>

namespace YAPT
{
	DescriptorSetUtility::DescriptorSetUtility()
		:m_gfx(YAPT_NULL_HANDLE),
		m_descSetLayoutHandle(YAPT_NULL_HANDLE),
		m_growAmount(8)
	{

	}
	DescriptorSetUtility::~DescriptorSetUtility()
	{
		deinit();
	}

	void DescriptorSetUtility::init(GfxApiHandle gfx, DescriptorSetLayoutHandle layoutHandle)
	{
		m_gfx = gfx;
		m_descSetLayoutHandle = layoutHandle;

	}

	void DescriptorSetUtility::preAllocate(size_t num)
	{
		allocateNewDescSetBatch(num);
	}


	void DescriptorSetUtility::deinit()
	{
		for (DescriptorSetPoolHandle descSetpool : m_descSetPools)
		{
			Gfx::destroyDescriptorSetPool(m_gfx, descSetpool);
		}
		m_descSetPools.clear();
	}

	void DescriptorSetUtility::freeDescriptorSet(DescriptorSetHandle handle)
	{
		Gfx::freeDescriptorSet(handle);
		m_pendingDescriptors.push_back(handle);
	}
	DescriptorSetHandle DescriptorSetUtility::getNewDescriptorSet()
	{
		DescriptorSetHandle handle = YAPT_NULL_HANDLE;

		if (m_freeDescriptors.size() > 0)
		{
			handle = m_freeDescriptors.back();
			m_freeDescriptors.pop_back();
		}

		if (handle == YAPT_NULL_HANDLE)
		{
			if (m_pendingDescriptors.size() > 0)
			{
				while (m_pendingDescriptors.size() > 0 && Gfx::isDescriptorSetUnused(m_pendingDescriptors.front()))
				{
					m_freeDescriptors.push_back(m_pendingDescriptors.front());
					m_pendingDescriptors.pop_front();
				}

				if (m_freeDescriptors.size() > 0)
				{
					handle = m_freeDescriptors.back();
					m_freeDescriptors.pop_back();
				}
			}
		}
		

		if (handle == YAPT_NULL_HANDLE)
		{
			//if were here, all the descriptors are in use or pending, create new pool
			allocateNewDescSetBatch(m_growAmount);
			assert(m_freeDescriptors.size() > 0);
			handle = m_freeDescriptors.back();
			m_freeDescriptors.pop_back();
		}
		

		Gfx::useDescriptorSet(handle);
		
		return handle;
	}


	void DescriptorSetUtility::allocateNewDescSetBatch(size_t count)
	{
		DescriptorSetPoolHandle pool = Gfx::createDescriptorSetPool(m_gfx, m_descSetLayoutHandle, count);
		m_descSetPools.push_back(pool);

		m_freeDescriptors.reserve(m_freeDescriptors.size() + count);

		for (size_t i = 0; i < count; ++i)
		{
			m_freeDescriptors.push_back(Gfx::getDescriptorSet(pool, i));
		}

	}
}