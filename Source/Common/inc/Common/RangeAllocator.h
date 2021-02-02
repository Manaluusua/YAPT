#pragma once

#include <unordered_map>
#include <shared_mutex>
namespace YAPT
{
	class RangeAllocator
	{
	public:
		struct Range
		{
			size_t offset;
			size_t numElements;
		};


		RangeAllocator(size_t totalNumberOfElements);
		~RangeAllocator();

		Range allocate(size_t numberOfElements);
		void free(Range* ranges, size_t numberOfRanges);

		bool isValidRange(Range range) { return range.numElements > 0; };

	private:
		size_t m_totalNumberOfElements;
		size_t m_currentHead;

		std::unordered_map<size_t, std::vector<Range>> m_freeRanges;
		std::mutex m_mutex;
	};
}