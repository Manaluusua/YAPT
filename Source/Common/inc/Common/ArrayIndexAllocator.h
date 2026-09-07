#pragma once

#include <vector>
#include <atomic>

namespace YAPT
{
	class ArrayIndexAllocator
	{
	public:

		const static uint32_t INVALID_INDEX = uint32_t(-1);

		ArrayIndexAllocator(uint32_t numberOf32bitMasks);
		virtual ~ArrayIndexAllocator();

		uint32_t allocate();
		void free(uint32_t index);

	protected:
		std::vector<std::atomic<uint32_t>> m_bitMasks;
		std::atomic<uint32_t> m_lastAllocatedBitmaskIndex;

	};
}