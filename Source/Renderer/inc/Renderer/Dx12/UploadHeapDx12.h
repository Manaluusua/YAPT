#pragma once

#include <Renderer/Dx12/Dx12CommonIncludes.h>
#include <Renderer/Dx12/FenceHelperDx12.h>
#include <Renderer/Shared/GfxTypes.h>
#include <renderer/Dx12/Dx12MiscUtils.h>
#include <vector>
#include <atomic>

namespace YAPT
{
	class ResourceManagerDx12;

	class UploadHeapDx12
	{
	public:

		UploadHeapDx12(ResourceManagerDx12& resourceMngr, size_t heapSize, size_t numberOfPartitions);
		~UploadHeapDx12();

		UploadHeapDx12(const UploadHeapDx12&) = delete;

		void nextPartition();

		bool allocate(size_t size, UploadUtility::UploadHeapAllocationInfo& heapAllocation);

	private:

		//upload heap stuff
		ResourceManagerDx12& m_resMngr;
		RCPtr<ID3D12Heap> m_uploadHeap;
		RCPtr<ID3D12Resource> m_uploadBuffer;
		char* m_mappedUploadBufferPtr;
		size_t m_heapSize;
		size_t m_numberOfPartitions;

		std::atomic<size_t> m_currentlyUsedPartitionSize;
		size_t m_sizePerPartition;
		size_t m_currentPartitionIndex;
	};

	










}