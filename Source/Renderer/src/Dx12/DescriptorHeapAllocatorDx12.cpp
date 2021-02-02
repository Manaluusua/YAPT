#include <Renderer/Dx12/DescriptorHeapAllocatorDx12.h>
#include <Renderer/Dx12/ResourceManagerDx12.h>
#include <Renderer/Dx12/DescriptorHeapDx12.h>

namespace YAPT
{
	DescriptorHeapAllocatorDx12::DescriptorHeapAllocatorDx12(ResourceManagerDx12& resMngr, D3D12_DESCRIPTOR_HEAP_TYPE type, size_t descriptorCount)
		:m_resMngr(resMngr),
		m_heap(resMngr.createDescriptorHeap(type, descriptorCount)),
		m_rangeAllocator(descriptorCount)
	{
		
		m_tempRangeArray.reserve(64);
	}
	DescriptorHeapAllocatorDx12::~DescriptorHeapAllocatorDx12()
	{
		m_heap->destroy();
	}

	RangeAllocator::Range DescriptorHeapAllocatorDx12::allocate(size_t count)
	{
		return m_rangeAllocator.allocate(count);
	}

	void DescriptorHeapAllocatorDx12::release(const RangeAllocator::Range& range, size_t releaseFrame)
	{
		std::unique_lock<std::mutex> lock(m_mutex);
		m_pendingRelease.emplace_back(releaseFrame, range);
	}
	void DescriptorHeapAllocatorDx12::flushReleasedRanges(size_t upToFrame)
	{
		{
			std::unique_lock<std::mutex> lock(m_mutex);
			while (!m_pendingRelease.empty())
			{
				if (m_pendingRelease.front().first <= upToFrame)
				{
					m_tempRangeArray.push_back(m_pendingRelease.front().second);
					m_pendingRelease.pop_front();
				}
				else
				{
					break;
				}
			}
		}
		
		
		if (m_tempRangeArray.size() > 0)
		{
			m_rangeAllocator.free(m_tempRangeArray.data(), m_tempRangeArray.size());
			m_tempRangeArray.clear();
		}

	}

}