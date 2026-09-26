#pragma once

#include <Gfx/GfxTypes.h>
#include <Gfx/Dx12/Dx12CommonIncludes.h>
#include <Gfx/Dx12/FenceHelperDx12.h>
#include <Gfx/Dx12/SubmissionThreadDx12.h>
#include <vector>

namespace YAPT
{
	class ResourceManagerDx12;

	class DownloadHelperDx12
	{

	public:

		DownloadHelperDx12(ResourceManagerDx12& resourceMngr, SubmissionThreadDx12* submissionThread, size_t numberOfPartitions);
		~DownloadHelperDx12();

		//the copies read what the frame is rendering, so they are only taken over here and issued once the render work
		//they depend on has been submitted
		void queueDownloads(const ReadbackDefinitions& def, FenceHandle fenceToSignal);
		bool hasQueuedDownloads() const { return m_fencesToSignal.size() > 0; }

		//records all queued copies into a single commandlist and submits it to the graphics queue, which orders it after
		//the frame's render work. frameIndex selects the allocator, which is safe to reuse once the frame cycle comes back
		//around to it since the copies are submitted before the frame fence is signaled.
		bool flushDownloadBatches(size_t frameIndex);

	private:

		void addTransition(ID3D12Resource* resource, UINT subresource, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after);

		ResourceManagerDx12& m_resMngr;
		SubmissionThreadDx12* m_submissionThread;

		std::vector<RCPtr<ID3D12CommandAllocator>> m_commandAllocators;
		RCPtr<GraphicsCommandListDx12> m_commandList;
		SubmissionThreadDx12::SubmissionId m_lastSubmission;

		//readbacks are only ever queued from the thread executing the render graph, so these do not need the multi
		//producer pending lists the uploads use
		std::vector<TextureReadbackDefinition> m_queuedTextureReadbacks;
		std::vector<BufferReadbackDefinition> m_queuedBufferReadbacks;
		//one per queueDownloads() call, the target value is captured at queue time
		std::vector<FenceValueDx12> m_fencesToSignal;

		std::vector<D3D12_RESOURCE_BARRIER> m_toCopyBarriers;
		std::vector<D3D12_RESOURCE_BARRIER> m_restoreBarriers;
	};

}
