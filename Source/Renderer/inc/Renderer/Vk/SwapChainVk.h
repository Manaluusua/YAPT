#pragma once

#include <Renderer/Vk/ResourceHandlesVk.h>
#include <Renderer/Vk/SyncUtilities.h>

namespace YAPT
{
	class RendererVk;

	class SwapChainVk
	{
	public:
		static VkSurfaceKHR createVkSurface(VkInstance instance, YaptRenderSurfaceHandle handle);
		static SwapChainVk* createSwapChain(RendererVk* rendererVk, size_t width, size_t height, size_t numberOfImages, VkSurfaceKHR surface);

		~SwapChainVk();

		void acquireNextFrame();
		VkSemaphore getBeforeUsageSemaphoreCurrentFrame() const;
		VkSemaphore getAfterUsageSemaphoreCurrentFrame() const;
		TextureHandle getTextureHandleCurrentFrame() const;

		void present(VkQueue presentQueue);

	private:
		SwapChainVk(RendererVk* rendererVk, VkSurfaceFormatKHR surfaceFormat, VkPresentModeKHR presentMode, uint32_t width, uint32_t height, VkSwapchainKHR swapChain);

		std::vector<VkImage> m_swapChainImages;
		std::vector<TextureHandleVk*> m_textureHandles;
		RingSyncUtility m_beforeUsageSemaphores;
		RingSyncUtility m_afterUsageSemaphores;
		VkDevice m_device;
		VkSurfaceFormatKHR m_surfaceFormat;
		VkPresentModeKHR m_presentMode;
		VkSwapchainKHR m_swapChain;
		uint32_t m_width;
		uint32_t m_height;
		uint32_t m_currentImageIndex;
	};

	



}