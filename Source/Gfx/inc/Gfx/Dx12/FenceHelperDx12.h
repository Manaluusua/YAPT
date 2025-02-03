#ifndef YAPT_DX12_FENCEHELPER_H
#define YAPT_DX12_FENCEHELPER_H


#include <Gfx/Dx12/Dx12CommonIncludes.h>
#include <vector>

namespace YAPT
{
	struct FenceState
	{
		ID3D12Fence* fence;
		UINT64 value;
	};

	class FenceHelperDx12
	{

	public:
		FenceHelperDx12();
		~FenceHelperDx12();

		bool initialize(ID3D12Device5* device, size_t numberOfFences);
		void deinitialize();

		void waitForFenceValueCPU(size_t fenceIndex, UINT64 value, DWORD duration = INFINITE);
		void waitForFenceValueGPU(ID3D12CommandQueue* commandQueue, size_t fenceIndex, UINT64 value);
		void signalFenceValueCPU(size_t fenceIndex, UINT64 value);
		void signalFenceValueGPU(ID3D12CommandQueue* commandQueue, size_t fenceIndex, UINT64 value);

		size_t getNumberOfFences() const { return m_fences.size(); }
		ID3D12Fence* getFence(size_t index) const { return m_fences[index].get(); }
	protected:
		std::vector<RCPtr<ID3D12Fence>> m_fences;
		HANDLE m_fenceEvent;
	};

	/*
		Assumes a frame cycle where every frame awaits for a free fence (in round robin fashion) and signals it when done.
	*/
	class FrameCycleFenceHelper
	{
	public:
		FrameCycleFenceHelper();
		~FrameCycleFenceHelper();

		bool initialize(ID3D12Device5* device, size_t numberOfFramesInCycle);
		void deinitialize();

		void waitForAllFramesToFinishCPU();
		void resetFrameCount();

		void waitForNextFrameCPU();
		void signalCurrentFrameGPU(ID3D12CommandQueue* commandQueue);

		ID3D12Fence* getFenceForFrame(size_t frameIndex);
		void incrementFrameCount();

		UINT64 getCurrentFrameIndex() const { return m_currentFrameIndex; }
		UINT64 getCurrentFrameCount() const { return m_currentFrameCount; }

	private:
		FenceHelperDx12 m_fenceHelper;
		size_t m_currentFrameCount;
		size_t m_currentFrameIndex;
		size_t m_framesInFlight;
	};

}
#endif