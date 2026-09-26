#include <Gfx/Dx12/SwapChainDx12.h>
#include <Gfx/Dx12/ResourceManagerDx12.h>
#include <Gfx/Dx12/DescriptorHeapDx12.h>
#include <dxgi1_4.h>


namespace YAPT
{
	SwapChainDx12::SwapChainDx12(ResourceManagerDx12& resMngr, IDXGIFactory4& factory)
		:m_resourceManager(resMngr),
		m_factory(factory)
		
	{

	}
	SwapChainDx12::~SwapChainDx12()
	{
		releaseSwapChain();
	}

	bool SwapChainDx12::setupSwapChain(HWND windowHandle, ID3D12CommandQueue& cmdQueue, size_t bufferCount, size_t width, size_t height)
	{
		releaseSwapChain();

		m_swapChainWidth = width;
		m_swapChainHeight = height;
		m_swapChainFrameCount = bufferCount;
		// Describe and create the swap chain.
		DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
		swapChainDesc.BufferCount = (UINT)m_swapChainFrameCount;
		swapChainDesc.Width = (UINT)m_swapChainWidth;
		swapChainDesc.Height = (UINT)m_swapChainHeight;
		swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		swapChainDesc.SampleDesc.Count = 1;



		RCPtr<IDXGISwapChain1> swapChain;
		if (FAILED(m_factory.CreateSwapChainForHwnd(
			&cmdQueue,
			windowHandle,
			&swapChainDesc,
			nullptr,
			nullptr,
			&swapChain
		)))
		{
			return false;
		}

		if (FAILED(m_factory.MakeWindowAssociation(windowHandle, DXGI_MWA_NO_ALT_ENTER)))
		{
			return false;
		}

		swapChain->QueryInterface(IID_PPV_ARGS(&m_swapChain));


		if (!m_swapChain.get())
		{
			return false;
		}
		m_currentFrameIndex = m_swapChain->GetCurrentBackBufferIndex();

		{
			D3D12_RENDER_TARGET_VIEW_DESC rtDesc;
			rtDesc.Texture2D.PlaneSlice = 0;
			rtDesc.Texture2D.MipSlice = 0;
			rtDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
			rtDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;

			m_swapChainImages.reserve(m_swapChainFrameCount);

			for (UINT i = 0; i < m_swapChainFrameCount; i++)
			{
				TextureHandleDx12* texHandle = new TextureHandleDx12;
				m_swapChainImages.push_back(texHandle);

				if (FAILED(m_swapChain->GetBuffer(i, IID_PPV_ARGS(&texHandle->resource))))
				{
					return false;
				}

				std::wstring name(L"Backbuffer");
				name += std::to_wstring(i);
				texHandle->resource->SetName(name.c_str());
				
				texHandle->lastSeenState.init(D3D12_RESOURCE_STATE_PRESENT, 1);
#ifdef DX12_DEBUGNAMES_ENABLE
				texHandle->name = "Swapchain Image";
#endif
				texHandle->dimension = ResourceDimension::TEXTURE_2D;
				texHandle->heapType = D3D12_HEAP_TYPE_DEFAULT;

				//matches swapChainDesc.Format above
				texHandle->texDesc = TextureDesc(ResourceDimension::TEXTURE_2D, ResourceFormat::RGBA8_UNORM, RESOURCE_USAGE_RENDER_TARGET_TEXTURE,
					swapChainDesc.Width, swapChainDesc.Height, 1, 1, MemoryType::DEFAULT);

				texHandle->textureDesc.Width = swapChainDesc.Width;
				texHandle->textureDesc.Height = swapChainDesc.Height;
				texHandle->textureDesc.Format = swapChainDesc.Format;
				texHandle->textureDesc.DepthOrArraySize = 1;
				texHandle->textureDesc.MipLevels = 1;
				texHandle->textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
				texHandle->textureDesc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
				texHandle->textureDesc.SampleDesc.Count = 1;
				texHandle->textureDesc.SampleDesc.Quality = 0;
				texHandle->textureDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
				texHandle->textureDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

			}
		}

		return true;
	}
	void SwapChainDx12::releaseSwapChain()
	{
		for (size_t i = 0; i < m_swapChainImages.size(); ++i)
		{
			delete m_swapChainImages[i];
		}

		m_swapChainImages.clear();
		m_swapChain = nullptr;
	}

	TextureHandleDx12* SwapChainDx12::acquireNextBackBuffer()
	{
		TextureHandleDx12* texHandle = m_swapChainImages[m_currentFrameIndex];
		m_currentFrameIndex = ++m_currentFrameIndex % m_swapChainFrameCount;
		return texHandle;
	}


	bool SwapChainDx12::present(UINT syncInterval, UINT flags)
	{
		bool success = SUCCEEDED(m_swapChain->Present(syncInterval, flags));
		return success;
	}

}