#pragma once


#include <Common/GrowingMultiProducerPendingList.h>
#include <Gfx/Dx12/UploadHeapDx12.h>
#include <Gfx/Dx12/SubmissionThreadDx12.h>
namespace YAPT
{

	class UploadHelperDx12
	{
		
	public:

		UploadHelperDx12(ResourceManagerDx12& resourceMngr, SubmissionThreadDx12* submissionThread, size_t heapSize, size_t numberOfPartitions);
		~UploadHelperDx12();

		void* mapCopyRangeForBufferData(ID3D12Resource* buffer, size_t offsetInBytes, size_t sizeInBytes);
		void uploadDataForBuffer(ID3D12Resource* buffer, size_t offsetInBytes, size_t sizeInBytes, const void* data);

		void uploadDataForTexture(ID3D12Resource* texture, const D3D12_RESOURCE_DESC& resourceDesc,
			size_t arraySliceOffset, size_t arraySliceCount, size_t mipOffset, size_t mipCount, const TextureDataDefinition* textureDataDefinitions, const ResourceStateDescription& afterUploadUsage);

		void prepareNextUploadBatch();
		void flushUploadBatch(FenceValueDx12* fencesToWait, size_t fenceCount, size_t queueIndex);

		FrameCycleFenceHelper& getFenceHelper() { return m_fenceHelper; }

	private:

		bool getHeapMemory(size_t sizeRequested, UploadUtility::UploadHeapAllocationInfo& info);

		void getHeapMemoryFromTemporaryHeap(size_t sizeRequested, UploadUtility::UploadHeapAllocationInfo& info);
		

		ResourceManagerDx12& m_resMngr;
		std::vector<RCPtr<ID3D12CommandAllocator> > m_commandAllocators;
		RCPtr<ID3D12GraphicsCommandList> m_commandList;
		FrameCycleFenceHelper m_fenceHelper;
		UploadHeapDx12 m_uploadHeap;
		SubmissionThreadDx12::SubmissionId m_lastUploadSubmission;
		GrowingMultiProducerPendingList<UploadUtility::BufferDataInfo> m_pendingBufferUploads;
		GrowingMultiProducerPendingList<UploadUtility::TextureDataInfo> m_pendingTextureUploads;
		GrowingMultiProducerPendingList<ID3D12Resource*> m_pendingUnmaps;

		SubmissionThreadDx12* m_submissionThread;

	};

}