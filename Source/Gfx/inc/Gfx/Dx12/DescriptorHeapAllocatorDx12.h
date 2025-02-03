#pragma once
#include <Gfx/Dx12/d3dx12.h>
#include <Common/RangeAllocator.h>
#include <deque>
#include <mutex>
namespace YAPT
{
	class ResourceManagerDx12;
	class DescriptorHeapDx12;

	class DescriptorHeapAllocatorDx12
	{
	public:
		DescriptorHeapAllocatorDx12(ResourceManagerDx12& resMngr, D3D12_DESCRIPTOR_HEAP_TYPE type, size_t descriptorCount);
		~DescriptorHeapAllocatorDx12();

		DescriptorHeapDx12* getHeap() { return m_heap; }


		RangeAllocator::Range allocate(size_t count);
		void release(const RangeAllocator::Range& range, size_t releaseFrame);

		void flushReleasedRanges(size_t upToFrame);

	private:
		ResourceManagerDx12& m_resMngr;
		DescriptorHeapDx12* m_heap;
		RangeAllocator m_rangeAllocator;
		std::deque<std::pair<size_t, RangeAllocator::Range>> m_pendingRelease;
		std::mutex m_mutex;

		std::vector<RangeAllocator::Range> m_tempRangeArray;
	};
}