#ifndef YAPT_DX12_SWAPCHAIN_H
#define YAPT_DX12_SWAPCHAIN_H


#include <Gfx/GfxTypes.h>
#include <memory>
#include <d3d12.h>
#include <vector>
struct IDXGISwapChain3;
struct IDXGIFactory4;
namespace YAPT
{
	class DescriptorHeapDx12;
	class ResourceManagerDx12;
	class SwapChainDx12
	{
	public:

		SwapChainDx12(ResourceManagerDx12& resMngr, IDXGIFactory4& factory);
		~SwapChainDx12();

		bool setupSwapChain(HWND windowHandle, ID3D12CommandQueue& cmdQueue, size_t bufferCount, size_t width, size_t height);
		TextureHandleDx12* acquireNextBackBuffer();


		bool present(UINT syncInterval, UINT flags);
	private:
		void releaseSwapChain();

		IDXGIFactory4& m_factory;
		ResourceManagerDx12& m_resourceManager;
		RCPtr<IDXGISwapChain3> m_swapChain;
		mutable std::vector<TextureHandleDx12*> m_swapChainImages;
		size_t m_currentFrameIndex;
		size_t m_swapChainFrameCount;
		size_t m_swapChainWidth;
		size_t m_swapChainHeight;
	};
}
#endif