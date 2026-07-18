#include <Common/ArenaAllocator.h>
#include <Common/Logger.h>
#include <assert.h>

namespace YAPT
{
	ArenaAllocator::ArenaAllocator(size_t capacityInBytes)
		:m_data(new uint8_t[capacityInBytes]),
		m_capacity(capacityInBytes),
		m_offset(0)
	{

	}
	ArenaAllocator::~ArenaAllocator()
	{
		delete[] m_data;
	}

	void* ArenaAllocator::allocate(size_t sizeInBytes, size_t alignment)
	{
		assert(alignment != 0 && (alignment & (alignment - 1)) == 0); //must be power of two

		size_t current = m_offset.load();
		while (true)
		{
			size_t alignedOffset = align(current, alignment);
			size_t newOffset = alignedOffset + sizeInBytes;

			if (newOffset > m_capacity)
			{
				YAPT_LOG_ERROR("ArenaAllocator: failed to allocate %zu bytes, out of memory!", sizeInBytes);
				return nullptr;
			}

			if (m_offset.compare_exchange_strong(current, newOffset))
			{
				return m_data + alignedOffset;
			}
		}
	}

	void ArenaAllocator::reset()
	{
		m_offset.store(0);
	}
}
