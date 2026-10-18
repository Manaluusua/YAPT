#include <Common/ChunkedArenaAllocator.h>
#include <assert.h>
#include <string.h>
#include <algorithm>

namespace YAPT
{
	ChunkedArenaAllocator::ChunkedArenaAllocator(size_t defaultBlockSizeInBytes)
		:m_currentBlock(0),
		m_defaultBlockSize(defaultBlockSizeInBytes)
	{

	}
	ChunkedArenaAllocator::~ChunkedArenaAllocator()
	{
		releaseBlocks();
	}

	void* ChunkedArenaAllocator::allocate(size_t sizeInBytes, size_t alignment)
	{
		assert(alignment != 0 && (alignment & (alignment - 1)) == 0); //must be power of two

		while (m_currentBlock < m_blocks.size())
		{
			Block& block = m_blocks[m_currentBlock];
			size_t alignedOffset = align(block.offset, alignment);
			if (alignedOffset + sizeInBytes <= block.capacity)
			{
				block.offset = alignedOffset + sizeInBytes;
				return block.data + alignedOffset;
			}
			++m_currentBlock;
		}

		addBlock(sizeInBytes + alignment);
		m_currentBlock = m_blocks.size() - 1;

		Block& block = m_blocks[m_currentBlock];
		size_t alignedOffset = align(block.offset, alignment);
		assert(alignedOffset + sizeInBytes <= block.capacity);
		block.offset = alignedOffset + sizeInBytes;
		return block.data + alignedOffset;
	}

	const char* ChunkedArenaAllocator::copyString(const char* str)
	{
		if (str == nullptr) str = "";
		size_t len = strlen(str);
		char* dst = static_cast<char*>(allocate(len + 1, 1));
		memcpy(dst, str, len + 1);
		return dst;
	}

	void ChunkedArenaAllocator::reset()
	{
		if (m_blocks.size() > 1)
		{
			size_t totalCapacity = getCapacity();
			releaseBlocks();
			addBlock(totalCapacity);
		}

		for (Block& block : m_blocks)
		{
			block.offset = 0;
		}
		m_currentBlock = 0;
	}

	size_t ChunkedArenaAllocator::getUsedBytes() const
	{
		size_t used = 0;
		for (const Block& block : m_blocks)
		{
			used += block.offset;
		}
		return used;
	}

	size_t ChunkedArenaAllocator::getCapacity() const
	{
		size_t capacity = 0;
		for (const Block& block : m_blocks)
		{
			capacity += block.capacity;
		}
		return capacity;
	}

	void ChunkedArenaAllocator::addBlock(size_t minimumSizeInBytes)
	{
		Block block;
		block.capacity = std::max(minimumSizeInBytes, m_defaultBlockSize);
		block.data = new uint8_t[block.capacity];
		block.offset = 0;
		m_blocks.push_back(block);
	}

	void ChunkedArenaAllocator::releaseBlocks()
	{
		for (Block& block : m_blocks)
		{
			delete[] block.data;
		}
		m_blocks.clear();
		m_currentBlock = 0;
	}
}
