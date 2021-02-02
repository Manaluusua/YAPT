#pragma once

#include <Common/ArrayIndexAllocator.h>

namespace YAPT
{
	class DeferredFreeArrayIndexAllocator : private ArrayIndexAllocator
	{
	public:
		DeferredFreeArrayIndexAllocator(size_t numberOfPendingFreeLists, size_t numberOf32bitMasks);
		~DeferredFreeArrayIndexAllocator();

		uint32_t allocate();
		void free(uint32_t index);

		void flush();
		

	private:
		void pumpFreeWritesToNextList();
		void flushNextFreeList();

		std::vector<std::atomic<uint32_t>> m_deferredFreeLists;
		size_t m_currentWriteIndex;
		size_t m_nextFreeListToFlushIndex;
		size_t m_bitMasksPerFreeList;
		size_t m_numberOfFreeLists;
		
	};
}