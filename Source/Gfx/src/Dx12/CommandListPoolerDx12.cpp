#include <Gfx/Dx12/CommandListPoolerDx12.h>

namespace YAPT
{
	CommandListPoolerDx12::CommandListPoolerDx12(ID3D12Device5& device, size_t pipelineLength)
		:m_device(device),
		m_pipelineLength(pipelineLength),
		m_currentFrame(0)
	{

	}


	CommandListPoolerDx12::~CommandListPoolerDx12()
	{
		reset();
	}

	bool CommandListPoolerDx12::initialize(D3D12_COMMAND_LIST_TYPE type, size_t numberOfListsPerFrame)
	{
		reset();

		m_numberOfListsPerFrame = numberOfListsPerFrame;

		size_t allocatorCount = numberOfListsPerFrame * m_pipelineLength;

		m_commandList.resize(numberOfListsPerFrame);
		m_allocators.resize(allocatorCount);
		m_handles.resize(numberOfListsPerFrame);


		for (size_t i = 0; i < allocatorCount; ++i)
		{
			if (FAILED(m_device.CreateCommandAllocator(type, IID_PPV_ARGS(&m_allocators[i]))))
			{
				return false;
			}
		}


		for (size_t i = 0; i < numberOfListsPerFrame; ++i)
		{
			if (FAILED(m_device.CreateCommandList(0, type, m_allocators[i].get(), nullptr, IID_PPV_ARGS(&m_commandList[i]))))
			{
				return false;
			}
		}

		for (size_t i = 0; i < numberOfListsPerFrame; ++i)
		{
			if (FAILED(m_commandList[i]->Close()))
			{
				return false;
			}
		}

		

		return true;
	}

	size_t CommandListPoolerDx12::getNumberOfCommandListsPerFrame() const
	{
		return m_numberOfListsPerFrame;
	}
	RCPtr<ID3D12CommandAllocator>& CommandListPoolerDx12::getAllocator(size_t index)
	{
		return m_allocators[m_currentFrame * m_numberOfListsPerFrame + index];
	}
	RCPtr<GraphicsCommandListDx12>& CommandListPoolerDx12::getCommandList(size_t index)
	{
		return m_commandList[index];
	}

	void CommandListPoolerDx12::incrementFrame()
	{
		m_currentFrame = (m_currentFrame + 1) % m_pipelineLength;
	}

	void CommandListPoolerDx12::reset()
	{
		m_commandList.clear();
		m_allocators.clear();
	}
}