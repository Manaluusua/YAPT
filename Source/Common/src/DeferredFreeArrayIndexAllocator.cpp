#pragma once
#include <Common/DeferredFreeArrayIndexAllocator.h>
#include <assert.h>
namespace YAPT
{
	DeferredFreeArrayIndexAllocator::DeferredFreeArrayIndexAllocator(size_t numberOfPendingFreeLists, uint32_t numberOf32bitMasks)
		:ArrayIndexAllocator(numberOf32bitMasks),
		m_currentWriteIndex(0),
		m_nextFreeListToFlushIndex(numberOfPendingFreeLists - 1),
		m_bitMasksPerFreeList(numberOf32bitMasks),
		m_numberOfFreeLists(numberOfPendingFreeLists),
		m_deferredFreeLists(numberOfPendingFreeLists* numberOf32bitMasks)
	{
		for (size_t i = 0; i < m_deferredFreeLists.size(); ++i)
		{
			m_deferredFreeLists[i] = 0;
		}
	}
	DeferredFreeArrayIndexAllocator::~DeferredFreeArrayIndexAllocator()
	{

	}

	uint32_t DeferredFreeArrayIndexAllocator::allocate()
	{
		return ArrayIndexAllocator::allocate();
	}
	void DeferredFreeArrayIndexAllocator::free(uint32_t index)
	{
		uint32_t maskIndex = index / 32;
		uint32_t maskBit = index & 31;

		std::atomic<uint32_t>* writeList = &m_deferredFreeLists[m_currentWriteIndex * m_bitMasksPerFreeList];

		writeList[maskIndex].fetch_or(1 << maskBit);
	}

	void DeferredFreeArrayIndexAllocator::flush()
	{
		flushNextFreeList();
		pumpFreeWritesToNextList();
	}

	void DeferredFreeArrayIndexAllocator::pumpFreeWritesToNextList()
	{
		size_t nextWriteIndex = (m_currentWriteIndex + 1) % m_numberOfFreeLists;
		assert(nextWriteIndex != m_nextFreeListToFlushIndex);
		m_currentWriteIndex = nextWriteIndex;
	}
	void DeferredFreeArrayIndexAllocator::flushNextFreeList()
	{
		std::atomic<uint32_t>* listToFlush = &m_deferredFreeLists[m_nextFreeListToFlushIndex * m_bitMasksPerFreeList];
		for (size_t i = 0; i < m_bitMasksPerFreeList; ++i)
		{
			uint32_t v = listToFlush[i].load();
			if (v != 0)
			{
				m_bitMasks[i].fetch_or(v);
				listToFlush[i].store(0);
			}
			
		}

		m_nextFreeListToFlushIndex = (m_nextFreeListToFlushIndex + 1) % m_numberOfFreeLists;
	}
}