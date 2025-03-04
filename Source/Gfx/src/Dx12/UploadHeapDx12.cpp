#include <Gfx/Dx12/UploadHeapDx12.h>
#include <Gfx/Dx12/ResourceManagerDx12.h>
#include <Gfx/Dx12/Dx12MiscUtils.h>
#include <Gfx/Dx12/d3dx12.h>

namespace YAPT
{
	UploadHeapDx12::UploadHeapDx12(ResourceManagerDx12& resourceMngr, size_t heapSize, size_t numberOfPartitions)
		:m_resMngr(resourceMngr),
		m_heapSize(heapSize),
		m_numberOfPartitions(numberOfPartitions),
		m_currentlyUsedPartitionSize(0),
		m_sizePerPartition(heapSize/ numberOfPartitions),
		m_currentPartitionIndex(0)
	{
		//create upload heap and required resources
		CD3DX12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(heapSize);
		m_alloc = m_resMngr.allocate(bufferDesc, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&m_uploadBuffer));
		checkForDxError(m_uploadBuffer->Map(0, nullptr, reinterpret_cast<void**>(&m_mappedUploadBufferPtr)));

	}
	UploadHeapDx12::~UploadHeapDx12()
	{
		m_uploadBuffer->Unmap(0, nullptr);
		m_resMngr.addToPendingDestructionList(m_alloc);
		m_resMngr.addToPendingDestructionList(m_uploadBuffer.get());
	}


	

	bool UploadHeapDx12::allocate(size_t size, UploadUtility::UploadHeapAllocationInfo& heapAllocation)
	{
		size_t copyOffset = m_currentPartitionIndex * m_sizePerPartition;

		size_t currentlyUsedPartitionSize;
		while (true)
		{
			currentlyUsedPartitionSize = m_currentlyUsedPartitionSize.load();
			if (currentlyUsedPartitionSize + size > m_sizePerPartition)
			{
				return false;
			}

			if (m_currentlyUsedPartitionSize.compare_exchange_weak(currentlyUsedPartitionSize, currentlyUsedPartitionSize + size))
			{
				break;
			}
		}

		heapAllocation.offset = copyOffset + currentlyUsedPartitionSize;
		heapAllocation.uploadBuffer = m_uploadBuffer;
		heapAllocation.mappedUploadBufferPtr = m_mappedUploadBufferPtr;

		return true;
	}

	void  UploadHeapDx12::nextPartition()
	{
		m_currentPartitionIndex = (m_currentPartitionIndex + 1) % m_numberOfPartitions;
		m_currentlyUsedPartitionSize.store(0);
	}
}