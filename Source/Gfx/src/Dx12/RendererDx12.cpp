
#include <Gfx/Dx12/RendererDx12.h>
#include <Gfx/Dx12/SwapChainDx12.h>
#include <Gfx/Dx12/ResourceManagerDx12.h>
#include <Gfx/Dx12/AccelerationStructureBuilderDx12.h>
#include <Gfx/Dx12/CommandListPoolerDx12.h>

#include <dxgi1_4.h>
#include <dxcapi.h>
#include <Gfx/Dx12/d3dx12.h>
#include <array>

#define ENABLE_DEBUG_LAYERS_DX12
using Microsoft::WRL::ComPtr;

namespace YAPT
{
	RendererDx12::RendererDx12(const GfxApiInitConfig& config)
		:m_config(config)
	{

	}

	RendererDx12::~RendererDx12()
	{
		deinitialize();
	}

	bool RendererDx12::createDevice()
	{
#ifdef ENABLE_DEBUG_LAYERS_DX12

		{
			ComPtr<ID3D12Debug> debugController;
			if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
			{
				debugController->EnableDebugLayer();
			}
		}
#endif
		if (FAILED(CreateDXGIFactory1(IID_PPV_ARGS(&m_factory))))
		{
			return false;
		}

		ComPtr<IDXGIAdapter1> adapter;
		for (UINT adapterIndex = 0; DXGI_ERROR_NOT_FOUND != m_factory->EnumAdapters1(adapterIndex, &adapter); ++adapterIndex)
		{
			DXGI_ADAPTER_DESC1 desc;
			adapter->GetDesc1(&desc);

			if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
			{
				continue;
			}

			if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_12_0, _uuidof(ID3D12Device5), nullptr)))
			{
				break;
			}
		}
		if (!adapter)
		{
			return false;
		}


		if (FAILED(D3D12CreateDevice(
			adapter.Get(),
			D3D_FEATURE_LEVEL_12_0,
			IID_PPV_ARGS(&m_device)
		)))
		{
			return false;
		}

		m_adapter = adapter.Get();

		

		return true;

	}



	bool RendererDx12::createCoreResources()
	{
		//create Queues
		{
		
			D3D12_COMMAND_QUEUE_DESC queueDesc = {};
			queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
			queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

			if (FAILED(m_device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_mainGraphicsQueue))))
			{
				return false;
			}
		}

		{
			D3D12_COMMAND_QUEUE_DESC queueDesc = {};
			queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
			queueDesc.Type = D3D12_COMMAND_LIST_TYPE_COPY;

			if (FAILED(m_device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_copyQueue))))
			{
				return false;
			}
		}

#ifdef DX12_DEBUGNAMES_ENABLE
		m_mainGraphicsQueue->SetName(L"Graphics Queue");
		m_copyQueue->SetName(L"Copy Queue");
#endif

		//synchronization
		return m_fenceHelper.initialize(m_device.get(), m_config.pipelineLength);
	}

	CommandListPoolerDx12* RendererDx12::createCommandListPooler()
	{
		CommandListPoolerDx12* pooler = new CommandListPoolerDx12(*m_device, getResourceManager().getPipelineLength());
		m_commandListPoolers.push_back(pooler);
		return pooler;
	}
	void RendererDx12::destroyCommandListPooler(CommandListPoolerDx12* pooler)
	{
		m_commandListPoolers.erase(std::remove(m_commandListPoolers.begin(), m_commandListPoolers.end(), pooler), m_commandListPoolers.end());
		delete pooler;
	}

	bool RendererDx12::initialize()
	{
		if (!createDevice())
		{
			return false;
		}
		if (!createCoreResources())
		{
			return false;
		}

		//submission thread setup
		SubmissionThreadDx12::Configuration submissionConfig;
		submissionConfig.numberOfQueues[SubmissionThreadDx12::COMMANDQUEUETYPE_COMPUTE] = 0;
		submissionConfig.numberOfQueues[SubmissionThreadDx12::COMMANDQUEUETYPE_GRAPHICS] = 1;
		submissionConfig.numberOfQueues[SubmissionThreadDx12::COMMANDQUEUETYPE_COPY] = 1;

		ID3D12CommandQueue* copyQueues[] = { m_copyQueue.get() };
		ID3D12CommandQueue* graphicsQueues[] = { m_mainGraphicsQueue.get() };

		submissionConfig.queues[SubmissionThreadDx12::COMMANDQUEUETYPE_COMPUTE] = nullptr;
		submissionConfig.queues[SubmissionThreadDx12::COMMANDQUEUETYPE_GRAPHICS] = graphicsQueues;
		submissionConfig.queues[SubmissionThreadDx12::COMMANDQUEUETYPE_COPY] = copyQueues;

		m_submitThread.initialize(submissionConfig);

		m_resourceManager = std::make_unique<ResourceManagerDx12>(*m_device.get(), *m_adapter.get(), m_config.pipelineLength);
		bool success = m_resourceManager->initialize(&m_submitThread);
		m_accStructureBuilder = std::make_unique<AccelerationStructureBuilder>(*m_resourceManager);

		return success;
	}

	void RendererDx12::deinitialize()
	{
		m_submitThread.deinitialize();
		m_fenceHelper.deinitialize();

		m_mainGraphicsQueue = nullptr;
		m_copyQueue = nullptr;

		m_accStructureBuilder.reset();
		m_resourceManager.reset();

		m_device = nullptr;
		m_factory = nullptr;
	}

	SwapChainDx12* RendererDx12::createSwapChain(const WindowSurfaceDefinition& windowSurface)
	{
		SwapChainDx12* sc = new SwapChainDx12(*m_resourceManager, *m_factory);
		sc->setupSwapChain(windowSurface.windowHandle, *m_mainGraphicsQueue.get(), m_config.pipelineLength, windowSurface.width, windowSurface.height);
		return sc;
	}

	void RendererDx12::destroySwapChain(SwapChainDx12* swapChain)
	{
		delete swapChain;
	}

	void RendererDx12::present(SwapChainDx12* swapChain)
	{
		m_swapChainsToPresentAfterFrameEnd.push_back(swapChain);
	}

	void RendererDx12::prepare()
	{
		m_resourceManager->prepare();
		
	}

	void RendererDx12::waitForAllFramesDone()
	{
		m_fenceHelper.waitForAllFramesToFinishCPU();
	}
	
	void RendererDx12::renderBegin()
	{
		UINT64 frameId = m_fenceHelper.getCurrentFrameCount();

		size_t fenceCount = 0;
		FenceValueDx12 fenceStates[1];
		//fence for previous frame rendering 
		if (frameId > 1)
		{
			fenceStates[fenceCount].value = frameId - 1;
			fenceStates[fenceCount].fence = m_fenceHelper.getFenceForFrame(fenceStates[fenceCount].value);
			++fenceCount;
		}

		m_resourceManager->uploadPreFrameData(fenceStates, fenceCount);
	}

	void RendererDx12::executeBegin()
	{
		
		UINT64 frameId = m_fenceHelper.getCurrentFrameCount();
		size_t fenceCount = 0;
		FenceValueDx12 fenceStates[1];
		//fence for previous frame rendering 
		if (frameId > 1)
		{
			fenceStates[fenceCount].value = frameId - 1;
			fenceStates[fenceCount].fence = m_fenceHelper.getFenceForFrame(fenceStates[fenceCount].value);
			++fenceCount;
		}

		m_resourceManager->uploadFrameData(fenceStates, fenceCount);

		while (m_submitThread.isPending(m_lastSubmitId))
		{
			std::this_thread::sleep_for(std::chrono::microseconds(1));
		}

		m_fenceHelper.waitForNextFrameCPU();
	}

	void RendererDx12::submitCommandLists(CommandBufferHandle* buffers, size_t numberOfBuffers)
	{
		m_submittedCommandLists.reserve(m_submittedCommandLists.size() + numberOfBuffers);
		for (size_t i = 0; i < numberOfBuffers; ++i)
		{
			m_submittedCommandLists.push_back(buffers[i]->cmdList);
		}
		
	}

	void RendererDx12::executeEnd()
	{
		m_resourceManager->issueWaitForLatestUploads(m_submitThread, SubmissionThreadDx12::COMMANDQUEUETYPE_GRAPHICS, 0);
		m_lastSubmitId = m_submitThread.submit(SubmissionThreadDx12::COMMANDQUEUETYPE_GRAPHICS, 0, m_submittedCommandLists.data(), m_submittedCommandLists.size());
		m_submittedCommandLists.clear();

		if (m_resourceManager->hasPendingDownloads())
		{
			m_resourceManager->flushDownloads((size_t)m_fenceHelper.getCurrentFrameIndex());
		}

		//quick and dirty present. Should in reality handle presents just like commandbuffers: add them to some sequential commandlist and process it with submits.

		auto cb = [](void* ptr)
		{
			static_cast<SwapChainDx12*>(ptr)->present(0, 0);
		};
		for (size_t i = 0; i < m_swapChainsToPresentAfterFrameEnd.size(); ++i)
		{
			
			m_submitThread.issueCallback(cb, m_swapChainsToPresentAfterFrameEnd[i]);

		}
		m_swapChainsToPresentAfterFrameEnd.clear();

		for (size_t i = 0; i < m_commandListPoolers.size(); ++i)
		{
			m_commandListPoolers[i]->incrementFrame();
		}

		FenceValueDx12 fenceState{ m_fenceHelper.getFenceForFrame(m_fenceHelper.getCurrentFrameCount()), m_fenceHelper.getCurrentFrameCount() };
		m_submitThread.signal(SubmissionThreadDx12::COMMANDQUEUETYPE_GRAPHICS, 0, &fenceState, 1);
		m_fenceHelper.incrementFrameCount();

	}


}