#include <Common/ArrayIndexAllocator.h>
#include <Common/CommonUtilities.h>

namespace YAPT
{


	ArrayIndexAllocator::ArrayIndexAllocator(uint32_t numberOf32bitMasks)
		:m_lastAllocatedBitmaskIndex(0),
		m_bitMasks(numberOf32bitMasks)
	{
		for (size_t i = 0; i < m_bitMasks.size(); ++i)
		{
			m_bitMasks[i] = 0xFFFFFFFF;
		}
	}
	ArrayIndexAllocator::~ArrayIndexAllocator()
	{

	}

	uint32_t ArrayIndexAllocator::allocate()
	{
		uint32_t currentBitmaskIndex = m_lastAllocatedBitmaskIndex;
		uint32_t startedFromIndex = currentBitmaskIndex;
		while (true)
		{

			uint32_t mask = m_bitMasks[currentBitmaskIndex].load();

			if (mask == 0)
			{
				currentBitmaskIndex = (currentBitmaskIndex + 1) % m_bitMasks.size();
				//Went through all the entries, did not find a free slot
				if (startedFromIndex == currentBitmaskIndex)
				{
					return INVALID_INDEX;
				}
				continue;
			}

			uint32_t lsb = getLSB(mask);

			uint32_t newMask = (1 << lsb) ^ mask;
			if (!m_bitMasks[currentBitmaskIndex].compare_exchange_strong(mask, newMask))
			{
				continue;
			}

			m_lastAllocatedBitmaskIndex.store(currentBitmaskIndex);

			return uint32_t(currentBitmaskIndex * 32 + lsb);
		}
	}
	void ArrayIndexAllocator::free(uint32_t index)
	{
		uint32_t maskIndex = index / 32;
		uint32_t maskBit = index & 31;

		m_bitMasks[maskIndex].fetch_or(1 << maskBit);
	}
}
