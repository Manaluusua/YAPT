#pragma once

#include <Gfx/GfxTypes.h>
#include <deque>

namespace YAPT
{
	class Renderer;
	class DescriptorSetUtility
	{
	public:

		DescriptorSetUtility();
		~DescriptorSetUtility();

		void init(GfxApiHandle gfx, DescriptorSetLayoutHandle layoutHandle);
		void deinit();
		void freeDescriptorSet(DescriptorSetHandle handle);
		DescriptorSetHandle getNewDescriptorSet();

		void preAllocate(size_t num);

		void setGrowAmount(size_t v) { m_growAmount = v; }
	private:

		void allocateNewDescSetBatch(size_t count);

		std::vector<DescriptorSetPoolHandle> m_descSetPools;
		std::vector<DescriptorSetHandle> m_freeDescriptors;
		std::deque<DescriptorSetHandle> m_pendingDescriptors;

		DescriptorSetLayoutHandle m_descSetLayoutHandle;
		GfxApiHandle m_gfx;
		size_t m_growAmount;
	};


}