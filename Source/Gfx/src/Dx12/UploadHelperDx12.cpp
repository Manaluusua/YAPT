#include <Gfx/Dx12/UploadHelperDx12.h>
#include <Gfx/Dx12/ResourceManagerDx12.h>
#include <Gfx/Dx12/d3dx12.h>
#define INITIAL_MAX_COPY_ENTRIES_PER_COPY_TYPE 1024  


namespace YAPT
{
	UploadHelperDx12::UploadHelperDx12(ResourceManagerDx12& resourceMngr, SubmissionThreadDx12* submissionThread, size_t heapSize, size_t numberOfPartitions)
		:m_resMngr(resourceMngr),
		m_uploadHeap(resourceMngr, heapSize, numberOfPartitions),
		m_pendingBufferUploads(INITIAL_MAX_COPY_ENTRIES_PER_COPY_TYPE),
		m_pendingTextureUploads(INITIAL_MAX_COPY_ENTRIES_PER_COPY_TYPE),
		m_pendingUnmaps(128),
		m_submissionThread(submissionThread)
	{

		m_commandAllocators.clear();
		m_commandAllocators.resize(numberOfPartitions);


		for (size_t i = 0; i < numberOfPartitions; ++i)
		{
			if (FAILED(resourceMngr.getDevice().CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COPY, IID_PPV_ARGS(&m_commandAllocators[i]))))
			{
				assert(!"Failed to create command allocator");
			}

		}

		if (FAILED(resourceMngr.getDevice().CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_COPY, m_commandAllocators[0].get(), nullptr, IID_PPV_ARGS(&m_commandList))))
		{
			assert(!"Failed to create command allocator");
		}

		if (FAILED(m_commandList->Close()))
		{
			assert(!"Failed to close commandlist");
		}

		bool success = m_fenceHelper.initialize(&resourceMngr.getDevice(), numberOfPartitions);
		assert(success);
	}


	UploadHelperDx12::~UploadHelperDx12()
	{
		m_commandAllocators.clear();
		m_commandList = nullptr;
		m_fenceHelper.deinitialize();
	}

	void* UploadHelperDx12::mapCopyRangeForBufferData(ID3D12Resource* buffer, size_t offsetInBytes, size_t sizeInBytes)
	{
		auto allocCallback = [this](size_t size, UploadUtility::UploadHeapAllocationInfo& allocInfo) { return getHeapMemory(size, allocInfo); };

		UploadUtility::BufferDataInfo* info = m_pendingBufferUploads.add(1);
		void* ptrOut = nullptr;

		UploadUtility::mapCopyRangeForBufferUpload(m_resMngr.getDevice(), buffer, offsetInBytes, sizeInBytes, allocCallback, *info, ptrOut);
		return ptrOut;
	}
	void UploadHelperDx12::uploadDataForBuffer(ID3D12Resource* buffer, size_t offsetInBytes, size_t sizeInBytes, const void* data)
	{
		auto allocCallback = [this](size_t size, UploadUtility::UploadHeapAllocationInfo& allocInfo) { return getHeapMemory(size, allocInfo); };

		void* ptr;
		UploadUtility::BufferDataInfo* info = m_pendingBufferUploads.add(1);

		UploadUtility::mapCopyRangeForBufferUpload(m_resMngr.getDevice(), buffer, offsetInBytes, sizeInBytes, allocCallback, *info, ptr);
		memcpy(ptr, data, sizeInBytes);
	}

	void UploadHelperDx12::uploadDataForTexture(ID3D12Resource* texture, const D3D12_RESOURCE_DESC& resourceDesc,
		size_t arraySliceOffset, size_t arraySliceCount, size_t mipOffset, size_t mipCount, const TextureDataDefinition* textureDataDefinitions)
	{
		auto allocCallback = [this](size_t size, UploadUtility::UploadHeapAllocationInfo& allocInfo) { return getHeapMemory(size, allocInfo); };

		UploadUtility::TextureDataInfo* info = m_pendingTextureUploads.add(arraySliceCount * mipCount);
		UploadUtility::uploadDataForTexture(m_resMngr.getDevice(), texture, resourceDesc, arraySliceOffset, arraySliceCount, mipOffset, mipCount,textureDataDefinitions, allocCallback, info);
	}

	bool UploadHelperDx12::getHeapMemory(size_t sizeRequested, UploadUtility::UploadHeapAllocationInfo& info)
	{
		bool success = m_uploadHeap.allocate(sizeRequested, info);
		if (!success)
		{
			getHeapMemoryFromTemporaryHeap(sizeRequested, info);
		}
		return true;
	}

	void UploadHelperDx12::getHeapMemoryFromTemporaryHeap(size_t sizeRequested, UploadUtility::UploadHeapAllocationInfo& info)
	{
		RCPtr<ID3D12Heap> heap = m_resMngr.createResourceHeap(D3D12_HEAP_TYPE_UPLOAD, sizeRequested, D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS);
		ID3D12Resource* buffer;
		void* mappedPtr;

		CD3DX12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeRequested);
		checkForDxError(m_resMngr.getDevice().CreatePlacedResource(
			heap.get(),
			0,
			&bufferDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&buffer))
		);
		checkForDxError(buffer->Map(0, nullptr, reinterpret_cast<void**>(&mappedPtr)));

		info.mappedUploadBufferPtr = (char*)mappedPtr;
		info.offset = 0;
		info.uploadBuffer = buffer;

		*m_pendingUnmaps.add(1) = buffer;


		m_resMngr.addToPendingDestructionList(buffer);
		m_resMngr.addToPendingDestructionList(heap.get());
		buffer->Release();
	}

	void UploadHelperDx12::prepareNextUploadBatch()
	{
		m_fenceHelper.waitForNextFrameCPU();
		m_uploadHeap.nextPartition();
	}


	void UploadHelperDx12::flushUploadBatch(FenceState* fencesToWait, size_t fenceCount, size_t queueIndex)
	{
		//to be safe, make sure the previous submit has been processed so we can reuse the commandlist
		while (m_submissionThread->isPending(m_lastUploadSubmission))
		{
			std::this_thread::sleep_for(std::chrono::microseconds(1));
		}

		//unmap any pending unmaps
		ID3D12Resource** pendingUnmaps = m_pendingUnmaps.getAll();
		for (size_t i = 0; i < m_pendingUnmaps.count(); ++i)
		{
			pendingUnmaps[i]->Unmap(0, nullptr);
		}
		m_pendingUnmaps.clear();


		bool hasUploads = m_pendingBufferUploads.count() > 0 || m_pendingTextureUploads.count() > 0;

		if (!hasUploads)
		{
			return; //nothing to upload.
		}


		ID3D12CommandAllocator* allocator = m_commandAllocators[m_fenceHelper.getCurrentFrameIndex()].get();
		allocator->Reset();
		m_commandList->Reset(allocator, nullptr);

		UploadUtility::TextureDataInfo* textureUploadsList = m_pendingTextureUploads.getAll();
		UploadUtility::BufferDataInfo* bufferUploadsList = m_pendingBufferUploads.getAll();

		//issue buffer copies
		for (size_t i = 0; i < m_pendingBufferUploads.count(); ++i)
		{
			const UploadUtility::BufferDataInfo& info = bufferUploadsList[i];
			m_commandList->CopyBufferRegion(
				info.dstBuffer, info.dstOffset,
				info.srcBuffer, info.srcOffset,
				info.numBytes
			);
		}
		

		//issue texture copies
		for (size_t i = 0; i < m_pendingTextureUploads.count(); ++i)
		{
			const UploadUtility::TextureDataInfo& info = textureUploadsList[i];
			m_commandList->CopyTextureRegion(&info.dstLocation, info.dstX, info.dstY, info.dstZ, &info.srcLocation, nullptr);
		}
		
		m_pendingBufferUploads.clear();
		m_pendingTextureUploads.clear();
		
		m_commandList->Close();

		//if the copies need to wait, issue waits here
		m_submissionThread->wait(SubmissionThreadDx12::COMMANDQUEUETYPE_COPY, queueIndex, fencesToWait, fenceCount);

		//submit copies
		ID3D12CommandList* commandLists[] = { m_commandList };
		m_lastUploadSubmission = m_submissionThread->submit(SubmissionThreadDx12::COMMANDQUEUETYPE_COPY, queueIndex, commandLists, countOf(commandLists));

		//signal the copies
		ID3D12Fence* fence = m_fenceHelper.getFenceForFrame(m_fenceHelper.getCurrentFrameCount());
		FenceState fenceState{ fence, m_fenceHelper.getCurrentFrameCount() };
		m_submissionThread->signal(SubmissionThreadDx12::COMMANDQUEUETYPE_COPY, queueIndex, &fenceState, 1);
		m_fenceHelper.incrementFrameCount();

	}

}