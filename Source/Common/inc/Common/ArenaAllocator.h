#pragma once

#include <atomic>
#include <stdint.h>
#include <Common/CommonUtilities.h>

namespace YAPT
{
	//Thread-safe linear (bump-pointer) allocator over a fixed-size block of memory. Individual allocations cannot be freed;
	//the whole arena is reclaimed at once via reset().
	class ArenaAllocator
	{
	public:
		YAPT_NOCOPY(ArenaAllocator);

		ArenaAllocator(size_t capacityInBytes);
		~ArenaAllocator();

		void* allocate(size_t sizeInBytes, size_t alignment = alignof(std::max_align_t));
		void reset();

		size_t getCapacity() const { return m_capacity; }
		size_t getUsedBytes() const { return m_offset.load(); }

	private:
		uint8_t* m_data;
		size_t m_capacity;
		std::atomic<size_t> m_offset;
	};
}
