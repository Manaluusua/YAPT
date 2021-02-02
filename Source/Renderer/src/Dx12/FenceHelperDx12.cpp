#include <Renderer/Dx12/FenceHelperDx12.h>

namespace YAPT
{

	/////////////////////////////// FenceHelper Impl
	FenceHelperDx12::FenceHelperDx12()
	{

	}
	FenceHelperDx12::~FenceHelperDx12()
	{

	}

	bool FenceHelperDx12::initialize(ID3D12Device5* device, size_t numberOfFences)
	{
		//synchronization
		m_fences.resize(numberOfFences);
		for (size_t i = 0; i < numberOfFences; ++i)
		{
			if (FAILED(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fences[i]))))
			{
				return false;
			}

		}

		m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
		if (m_fenceEvent == nullptr)
		{
			return false;
		}

		return true;
	}
	void FenceHelperDx12::deinitialize()
	{
		if (m_fenceEvent)
		{
			CloseHandle(m_fenceEvent);
			m_fenceEvent = nullptr;
		}
	}

	void FenceHelperDx12::waitForFenceValueCPU(size_t fenceIndex, UINT64 value, DWORD duration)
	{
		//YAPT_LOG_DEBUG("checking for wait For %d on slot %d", value, fenceIndex);
		auto v = m_fences[fenceIndex]->GetCompletedValue();
		if (m_fences[fenceIndex]->GetCompletedValue() < value)
		{
			//YAPT_LOG_DEBUG("waiting for %d on slot %d", value, fenceIndex);
			m_fences[fenceIndex]->SetEventOnCompletion(value, m_fenceEvent);
			WaitForSingleObject(m_fenceEvent, duration);
			//YAPT_LOG_DEBUG("waiting for %d on slot %d :DONE", value, fenceIndex);
		}
	}
	void FenceHelperDx12::waitForFenceValueGPU(ID3D12CommandQueue* commandQueue, size_t fenceIndex, UINT64 value)
	{
		checkForDxError(commandQueue->Wait(m_fences[fenceIndex].get(), value));
	}
	void FenceHelperDx12::signalFenceValueCPU(size_t fenceIndex, UINT64 value)
	{
		m_fences[fenceIndex]->Signal(value);
	}
	void FenceHelperDx12::signalFenceValueGPU(ID3D12CommandQueue* commandQueue, size_t fenceIndex, UINT64 value)
	{
		checkForDxError(commandQueue->Signal(m_fences[fenceIndex].get(), value));
	}



	/////////////////////////////// FrameCycleFenceHelper Impl

	FrameCycleFenceHelper::FrameCycleFenceHelper()
		:m_currentFrameCount(0),
		m_currentFrameIndex(0)
	{
	}
	FrameCycleFenceHelper::~FrameCycleFenceHelper()
	{

	}

	bool FrameCycleFenceHelper::initialize(ID3D12Device5* device, size_t numberOfFramesInCycle)
	{
		
		m_currentFrameCount = 1;
		m_currentFrameIndex = 0;
		m_framesInFlight = numberOfFramesInCycle;
		return m_fenceHelper.initialize(device, 1);
	}

	void FrameCycleFenceHelper::deinitialize()
	{
		m_fenceHelper.deinitialize();
	}

	void FrameCycleFenceHelper::waitForAllFramesToFinishCPU()
	{
		if (m_currentFrameCount == 0)
		{
			return;
		}

		size_t lastFrame = m_currentFrameCount - 1;
		m_fenceHelper.waitForFenceValueCPU(0, lastFrame, INFINITE);
	}
	void FrameCycleFenceHelper::resetFrameCount()
	{
		waitForAllFramesToFinishCPU();
		m_currentFrameCount = 1;
		m_currentFrameIndex = 0;
	}

	void FrameCycleFenceHelper::waitForNextFrameCPU()
	{
		//first frames don't need sync since nothing is pending
		if (m_currentFrameCount <= m_framesInFlight)
		{
			return;
		}

		size_t fenceValueToWait = m_currentFrameCount - m_framesInFlight;
		m_fenceHelper.waitForFenceValueCPU(0, fenceValueToWait, INFINITE);


		
	}


	ID3D12Fence* FrameCycleFenceHelper::getFenceForFrame(size_t frameIndex)
	{
		return m_fenceHelper.getFence(0);
	}

	void FrameCycleFenceHelper::signalCurrentFrameGPU(ID3D12CommandQueue* commandQueue)
	{
		//YAPT_LOG_DEBUG("Signalling %d on slot %d", m_currentFrameCount, m_currentFrameIndex);
		m_fenceHelper.signalFenceValueGPU(commandQueue, 0, m_currentFrameCount);
		incrementFrameCount();
	}

	void FrameCycleFenceHelper::incrementFrameCount()
	{
		++m_currentFrameCount;
		m_currentFrameIndex = (m_currentFrameCount - 1) % m_framesInFlight;
	}
}