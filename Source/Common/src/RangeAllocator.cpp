#include <Common/RangeAllocator.h>
#include <Common/Logger.h>
namespace YAPT
{
	RangeAllocator::RangeAllocator(size_t totalNumberOfElements)
		:m_totalNumberOfElements(totalNumberOfElements),
		m_currentHead(0)
	{

	}
	RangeAllocator::~RangeAllocator()
	{

	}

	RangeAllocator::Range RangeAllocator::allocate(size_t numberOfElements)
	{
		Range retVal{size_t(-1),0};
		{
			std::unique_lock<std::mutex>(m_mutex);
			auto it = m_freeRanges.find(numberOfElements);
			if (it != m_freeRanges.end() && it->second.size() > 0)
			{
				retVal = it->second.back();
				it->second.pop_back();
			}
			else
			{
				if (m_totalNumberOfElements < (m_currentHead + numberOfElements))
				{
					YAPT_LOG_FATAL_ERROR("Failed to allocate Range!");
					return retVal;
				}


				retVal.offset = m_currentHead;
				retVal.numElements = numberOfElements;
				m_currentHead += numberOfElements;
			}
		}

		return retVal;
		
	}
	void RangeAllocator::free(Range* ranges, size_t numberOfRanges)
	{
		std::unique_lock<std::mutex>(m_mutex);

		for (size_t i = 0; i < numberOfRanges; ++i)
		{
			m_freeRanges[ranges[i].numElements].push_back(ranges[i]);
		}
	}
}