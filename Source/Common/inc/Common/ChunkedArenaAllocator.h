#pragma once

#include <stdint.h>
#include <vector>
#include <new>
#include <utility>
#include <Common/CommonUtilities.h>

namespace YAPT
{
	//growable single threaded bump allocator, does not run destructors
	class ChunkedArenaAllocator
	{
	public:
		YAPT_NOCOPY(ChunkedArenaAllocator);

		ChunkedArenaAllocator(size_t defaultBlockSizeInBytes = 64 * 1024);
		~ChunkedArenaAllocator();

		void* allocate(size_t sizeInBytes, size_t alignment = alignof(std::max_align_t));

		template<typename T, typename... Args>
		T* create(Args&&... args)
		{
			void* mem = allocate(sizeof(T), alignof(T));
			return new (mem) T(std::forward<Args>(args)...);
		}

		template<typename T>
		T* allocateArray(size_t count)
		{
			if (count == 0) return nullptr;
			T* data = static_cast<T*>(allocate(sizeof(T) * count, alignof(T)));
			for (size_t i = 0; i < count; ++i)
			{
				new (data + i) T();
			}
			return data;
		}

		template<typename T>
		T* copyArray(const T* src, size_t count)
		{
			if (count == 0) return nullptr;
			T* data = static_cast<T*>(allocate(sizeof(T) * count, alignof(T)));
			for (size_t i = 0; i < count; ++i)
			{
				new (data + i) T(src[i]);
			}
			return data;
		}

		const char* copyString(const char* str);

		void reset();

		size_t getNumberOfBlocks() const { return m_blocks.size(); }
		size_t getUsedBytes() const;
		size_t getCapacity() const;

	private:
		struct Block
		{
			uint8_t* data;
			size_t capacity;
			size_t offset;
		};

		void addBlock(size_t minimumSizeInBytes);
		void releaseBlocks();

		std::vector<Block> m_blocks;
		size_t m_currentBlock;
		size_t m_defaultBlockSize;
	};
}
