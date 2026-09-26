#include <Gfx/Dx12/DownloadHelperDx12.h>
#include <Gfx/Dx12/ResourceManagerDx12.h>
#include <Gfx/Dx12/d3dx12.h>
#include <thread>
#include <algorithm>

namespace YAPT
{
	DownloadHelperDx12::DownloadHelperDx12(ResourceManagerDx12& resourceMngr, SubmissionThreadDx12* submissionThread, size_t numberOfPartitions)
		:m_resMngr(resourceMngr),
		m_submissionThread(submissionThread)
	{
		m_commandAllocators.resize(numberOfPartitions);
		for (size_t i = 0; i < numberOfPartitions; ++i)
		{
			if (FAILED(resourceMngr.getDevice().CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_commandAllocators[i]))))
			{
				assert(!"Failed to create command allocator");
			}
		}

		if (FAILED(resourceMngr.getDevice().CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandAllocators[0].get(), nullptr, IID_PPV_ARGS(&m_commandList))))
		{
			assert(!"Failed to create commandlist");
		}

		if (FAILED(m_commandList->Close()))
		{
			assert(!"Failed to close commandlist");
		}

#ifdef DX12_DEBUGNAMES_ENABLE
		m_commandList->SetName(L"Readback commandlist");
#endif
	}

	DownloadHelperDx12::~DownloadHelperDx12()
	{
		m_commandList = nullptr;
		m_commandAllocators.clear();
	}

	void DownloadHelperDx12::queueDownloads(const ReadbackDefinitions& def, FenceHandle fenceToSignal)
	{
		if (def.textureReadbackCount == 0 && def.bufferReadbackCount == 0) return;

		if (def.textureReadbackCount > 0)
		{
			m_queuedTextureReadbacks.insert(m_queuedTextureReadbacks.end(), def.textureReadbackDefinitions, def.textureReadbackDefinitions + def.textureReadbackCount);
		}
		if (def.bufferReadbackCount > 0)
		{
			m_queuedBufferReadbacks.insert(m_queuedBufferReadbacks.end(), def.bufferReadbackDefinitions, def.bufferReadbackDefinitions + def.bufferReadbackCount);
		}


		FenceDx12* fence = m_resMngr.getFence(fenceToSignal);
		if (fence != nullptr)
		{
			m_fencesToSignal.push_back({ fence->fence.get(), fence->signalValue.load() });
		}
		else
		{
			//still need something marking that there is work queued
			m_fencesToSignal.push_back({ nullptr, 0 });
		}
	}

	void DownloadHelperDx12::addTransition(ID3D12Resource* resource, UINT subresource, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after)
	{

		if ((before & D3D12_RESOURCE_STATE_COPY_SOURCE) != 0)
		{
			return;
		}


		for (size_t i = 0; i < m_toCopyBarriers.size(); ++i)
		{
			const D3D12_RESOURCE_TRANSITION_BARRIER& t = m_toCopyBarriers[i].Transition;
			if (t.pResource == resource && t.Subresource == subresource)
			{
				return;
			}
		}

		m_toCopyBarriers.push_back(CD3DX12_RESOURCE_BARRIER::Transition(resource, before, after, subresource));
		m_restoreBarriers.push_back(CD3DX12_RESOURCE_BARRIER::Transition(resource, after, before, subresource));
	}

	bool DownloadHelperDx12::flushDownloadBatches(size_t frameIndex)
	{
		if (!hasQueuedDownloads()) return false;

		//make sure the previous submit has been processed so the commandlist can be reused
		while (m_submissionThread->isPending(m_lastSubmission))
		{
			std::this_thread::sleep_for(std::chrono::microseconds(1));
		}

		ID3D12CommandAllocator* allocator = m_commandAllocators[frameIndex].get();
		checkForDxError(allocator->Reset());
		checkForDxError(m_commandList->Reset(allocator, nullptr));

		m_toCopyBarriers.clear();
		m_restoreBarriers.clear();


		for (size_t i = 0; i < m_queuedTextureReadbacks.size(); ++i)
		{
			const TextureReadbackDefinition& info = m_queuedTextureReadbacks[i];
			assert(info.dst->heapType == D3D12_HEAP_TYPE_READBACK);

			for (uint32_t slice = 0; slice < info.def.srcSubresource.arraySliceCount; ++slice)
			{
				const UINT srcSub = info.src->getSubresourceIndex(info.def.srcSubresource.arraySliceOffset + slice, info.def.srcSubresource.mip);
				addTransition(info.src->resource, srcSub, info.src->lastSeenState.getStateForSubResource(srcSub), D3D12_RESOURCE_STATE_COPY_SOURCE);
			}
		}

		for (size_t i = 0; i < m_queuedBufferReadbacks.size(); ++i)
		{
			const BufferReadbackDefinition& info = m_queuedBufferReadbacks[i];
			assert(info.dst->heapType == D3D12_HEAP_TYPE_READBACK);

			addTransition(info.src->resource, 0, info.src->lastSeenState.getStateForSubResource(0), D3D12_RESOURCE_STATE_COPY_SOURCE);
		}

		if (m_toCopyBarriers.size() > 0)
		{
			m_commandList->ResourceBarrier((UINT)m_toCopyBarriers.size(), m_toCopyBarriers.data());
		}

		//buffer copies
		for (size_t i = 0; i < m_queuedBufferReadbacks.size(); ++i)
		{
			const BufferReadbackDefinition& info = m_queuedBufferReadbacks[i];
			m_commandList->CopyBufferRegion(info.dst->resource, info.def.dstOffset, info.src->resource, info.def.srcOffset, info.def.size);
		}

		//texture copies, one per array slice
		for (size_t i = 0; i < m_queuedTextureReadbacks.size(); ++i)
		{
			const TextureReadbackDefinition& info = m_queuedTextureReadbacks[i];

			D3D12_BOX srcBox;
			srcBox.left = (UINT)info.def.srcOffset.x;
			srcBox.top = (UINT)info.def.srcOffset.y;
			srcBox.front = (UINT)info.def.srcOffset.z;
			srcBox.right = srcBox.left + (UINT)info.def.extent.width;
			srcBox.bottom = srcBox.top + (UINT)info.def.extent.height;
			srcBox.back = srcBox.front + (UINT)info.def.extent.depth;

			for (uint32_t slice = 0; slice < info.def.srcSubresource.arraySliceCount; ++slice)
			{
				const UINT srcSub = info.src->getSubresourceIndex(info.def.srcSubresource.arraySliceOffset + slice, info.def.srcSubresource.mip);
				const UINT dstSub = info.dst->getSubresourceIndex(info.def.dstSubresource.arraySliceOffset + slice, info.def.dstSubresource.mip);
				assert(dstSub < info.dst->footprints.size());

				D3D12_TEXTURE_COPY_LOCATION srcLocation = {};
				srcLocation.pResource = info.src->resource;
				srcLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
				srcLocation.SubresourceIndex = srcSub;

				D3D12_TEXTURE_COPY_LOCATION dstLocation = {};
				dstLocation.pResource = info.dst->resource;
				dstLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
				dstLocation.PlacedFootprint = info.dst->footprints[dstSub];

				m_commandList->CopyTextureRegion(&dstLocation, (UINT)info.def.dstOffset.x, (UINT)info.def.dstOffset.y, (UINT)info.def.dstOffset.z, &srcLocation, &srcBox);
			}
		}

		if (m_restoreBarriers.size() > 0)
		{
			m_commandList->ResourceBarrier((UINT)m_restoreBarriers.size(), m_restoreBarriers.data());
		}

		checkForDxError(m_commandList->Close());

		ID3D12CommandList* commandLists[] = { m_commandList.get() };
		m_lastSubmission = m_submissionThread->submit(SubmissionThreadDx12::COMMANDQUEUETYPE_GRAPHICS, 0, commandLists, countOf(commandLists));

		//everything went into a single commandlist, so every fence can be signaled once it is done
		m_fencesToSignal.erase(std::remove_if(m_fencesToSignal.begin(), m_fencesToSignal.end(), [](const FenceValueDx12& f) { return f.fence == nullptr; }), m_fencesToSignal.end());
		if (m_fencesToSignal.size() > 0)
		{
			m_submissionThread->signal(SubmissionThreadDx12::COMMANDQUEUETYPE_GRAPHICS, 0, m_fencesToSignal.data(), m_fencesToSignal.size());
		}

		m_queuedTextureReadbacks.clear();
		m_queuedBufferReadbacks.clear();
		m_fencesToSignal.clear();

		return true;
	}
}
