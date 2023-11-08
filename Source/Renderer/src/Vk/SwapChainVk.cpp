#include <Renderer/Vk/SwapChainVk.h>
#include <Common/CommonUtilities.h>
#include <Renderer/Vk/RendererVk.h>
#include <Renderer/Vk/YaptToVkConversions.h>

#include <assert.h>
#include <WinUser.h>
namespace YAPT
{
	

	

	VkSurfaceFormatKHR selectSurfaceFormat(size_t supportedFormatsCount, const VkSurfaceFormatKHR* supportedFormats, ResourceFormat& selectedYaptFormat)
	{
		//for now just hardcode formats we want
		std::pair<VkSurfaceFormatKHR, ResourceFormat> formatsWanted[] = { 
			{{VK_FORMAT_R8G8B8A8_SRGB, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR}, {ResourceFormat::RGBA8_SRGB}}, 
			{{VK_FORMAT_B8G8R8A8_SRGB, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR}, {ResourceFormat::BGRA8_SRGB}}
		};
		VkSurfaceFormatKHR selectedFormat = { VK_FORMAT_UNDEFINED };

		for (size_t i = 0; i < (size_t)countOf(formatsWanted); ++i)
		{
			if (selectedFormat.format != VK_FORMAT_UNDEFINED) break;
			const VkSurfaceFormatKHR f = formatsWanted[i].first;
			selectedYaptFormat = formatsWanted[i].second;

			for (size_t k = 0; k < supportedFormatsCount; ++k)
			{
				if (supportedFormats[k].format == f.format && supportedFormats[k].colorSpace == f.colorSpace)
				{
					selectedFormat = supportedFormats[k];
					break;
				}
			}
		}
		return selectedFormat;
		
	}

	VkPresentModeKHR selectPresentMode(size_t supportedModesCount, const VkPresentModeKHR* supportedModes)
	{
		VkPresentModeKHR wantedModes[] = { VK_PRESENT_MODE_MAILBOX_KHR, VK_PRESENT_MODE_FIFO_RELAXED_KHR };
		for (size_t i = 0; i < (size_t)countOf(wantedModes); ++i)
		{
			for (size_t k = 0; k < supportedModesCount; ++k)
			{
				if (wantedModes[i] == supportedModes[k])
				{
					return wantedModes[i];
				}
			}
		}
		return VK_PRESENT_MODE_FIFO_KHR; //forced to be supported
	}

	VkSurfaceKHR SwapChainVk::createVkSurface(VkInstance instance, YaptRenderSurfaceHandle handle)
	{
		VkSurfaceKHR surface;
		VkWin32SurfaceCreateInfoKHR info{};
		info.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
		info.hwnd = handle;
		info.hinstance = (HINSTANCE)GetWindowLongPtr(info.hwnd, GWLP_HINSTANCE);
		VkResult res = vkCreateWin32SurfaceKHR(instance, &info, NULL, &surface);
		return surface;
	}

	ResourceFormat SwapChainVk::getPreferredSurfaceFormat(RendererVk* rendererVk, VkSurfaceKHR surface)
	{
		std::vector<VkSurfaceFormatKHR> supportedFormats;
		std::vector<VkPresentModeKHR> supportedPresentModes;
		VkSurfaceCapabilitiesKHR surfaceCaps;

		VkPhysicalDevice physicalDevice = rendererVk->getPhysicalDevice();
		VkDevice device = rendererVk->getDevice();

		VkResult res = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &surfaceCaps);
		checkForVkError(res);

		uint32_t supportedFormatCount = 0;
		vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &supportedFormatCount, nullptr);
		supportedFormats.resize(supportedFormatCount);
		res = vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &supportedFormatCount, supportedFormats.data());
		checkForVkError(res);


		uint32_t supportedPresentModeCount;
		vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &supportedPresentModeCount, nullptr);
		supportedPresentModes.resize(supportedPresentModeCount);
		res = vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &supportedPresentModeCount, supportedPresentModes.data());
		checkForVkError(res);

		ResourceFormat format;
		VkSurfaceFormatKHR selectedFormat = selectSurfaceFormat(supportedFormats.size(), supportedFormats.data(), format);
		
		return format;
	}

	SwapChainVk* SwapChainVk::createSwapChain(RendererVk* rendererVk, ResourceFormat format, size_t width, size_t height, size_t numberOfImages, VkSurfaceKHR surface)
	{
		
		std::vector<VkPresentModeKHR> supportedPresentModes;
		VkSurfaceCapabilitiesKHR surfaceCaps;

		VkPhysicalDevice physicalDevice = rendererVk->getPhysicalDevice();
		VkDevice device = rendererVk->getDevice();

		VkResult res = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &surfaceCaps);
		checkForVkError(res);

		uint32_t supportedPresentModeCount;
		vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &supportedPresentModeCount, nullptr);
		supportedPresentModes.resize(supportedPresentModeCount);
		res = vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &supportedPresentModeCount, supportedPresentModes.data());
		checkForVkError(res);

		VkSurfaceFormatKHR selectedFormat = { yaptFormatToVk(format), VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };

		VkPresentModeKHR selectedPresentMode = selectPresentMode(supportedPresentModes.size(), supportedPresentModes.data());

		VkExtent2D swapChainExtent = { clamp((uint32_t)width, surfaceCaps.minImageExtent.width, surfaceCaps.maxImageExtent.width), clamp((uint32_t)height, surfaceCaps.minImageExtent.height, surfaceCaps.maxImageExtent.height) };

		VkSwapchainCreateInfoKHR createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		createInfo.surface = surface;
		createInfo.minImageCount = (uint32_t)numberOfImages;
		createInfo.imageFormat = selectedFormat.format;
		createInfo.imageColorSpace = selectedFormat.colorSpace;
		createInfo.imageExtent = swapChainExtent;
		createInfo.imageArrayLayers = 1;
		createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE; //we assume the graphics queue using the swapchain images also presents them
		createInfo.queueFamilyIndexCount = 0; 
		createInfo.pQueueFamilyIndices = nullptr; 
		createInfo.preTransform = surfaceCaps.currentTransform;
		createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		createInfo.presentMode = selectedPresentMode;
		createInfo.clipped = VK_TRUE;
		createInfo.oldSwapchain = VK_NULL_HANDLE;


		VkSwapchainKHR swapChainVk;
		res = vkCreateSwapchainKHR(device, &createInfo, VK_ALLOC_CB, &swapChainVk);
		if (res != VK_SUCCESS)
		{
			return nullptr;
		}

		return new SwapChainVk(rendererVk, selectedFormat, format, selectedPresentMode, swapChainExtent.width, swapChainExtent.height, swapChainVk);
	}

	

	SwapChainVk::SwapChainVk(RendererVk* rendererVk, VkSurfaceFormatKHR surfaceFormat, ResourceFormat yaptFormat, VkPresentModeKHR presentMode, uint32_t width, uint32_t height, VkSwapchainKHR swapChain)
		:m_device(rendererVk->getDevice()),
		m_surfaceFormat(surfaceFormat),
		m_presentMode(presentMode),
		m_swapChain(swapChain),
		m_width(width),
		m_height(height),
		m_currentImageIndex(0),
		m_yaptFormat(yaptFormat)
	{
		uint32_t imageCount;
		vkGetSwapchainImagesKHR(m_device, swapChain, &imageCount, nullptr);
		m_swapChainImages.resize(imageCount);
		vkGetSwapchainImagesKHR(m_device, swapChain, &imageCount, m_swapChainImages.data());

		m_beforeUsageSemaphores.initialize(m_device, imageCount, false, true);
		m_afterUsageSemaphores.initialize(m_device, imageCount, false, true);

		m_textureHandles.reserve(imageCount);

		//wrap swapchain images to texturehandles
		{
			VkImageCreateInfo vkImageInfo{};
			vkImageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
			vkImageInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
			vkImageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
			vkImageInfo.format = m_surfaceFormat.format;
			vkImageInfo.imageType = VK_IMAGE_TYPE_2D;
			vkImageInfo.arrayLayers = 1;
			vkImageInfo.extent.width = width;
			vkImageInfo.extent.height = height;
			vkImageInfo.extent.depth = 1;
			vkImageInfo.mipLevels = 1;
			vkImageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
			vkImageInfo.samples = VK_SAMPLE_COUNT_1_BIT;

			VkMemoryPropertyFlags memoryFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

			for (uint32_t i = 0; i < imageCount; ++i)
			{
				VkImage vkImage = m_swapChainImages[i];

				TextureHandleVk* texHandle = new TextureHandleVk(*rendererVk->getResourceManager());
				texHandle->memoryFlags = memoryFlags;
				texHandle->createInfo = vkImageInfo;
				texHandle->dimensions = ResourceDimension::TEXTURE_2D;
				texHandle->currentLayouts.init(VK_IMAGE_LAYOUT_UNDEFINED, 1);
				texHandle->image = vkImage;
				texHandle->owningQueueFamily = rendererVk->getGraphicsQueue().queueFamilyIndex;
#ifdef VK_DEBUGNAMES_ENABLE
				texHandle->name = "SwapChain Image " + std::to_string(i);
#endif
				m_textureHandles.push_back(texHandle);
			}
		}
		

	}

	SwapChainVk::~SwapChainVk()
	{
		for (int i = 0; i < m_textureHandles.size(); ++i)
		{
			delete m_textureHandles[i];
		}
		vkDestroySwapchainKHR(m_device, m_swapChain, VK_ALLOC_CB);
	}


	void SwapChainVk::acquireNextFrame()
	{
		m_beforeUsageSemaphores.nextFrame();
		m_afterUsageSemaphores.nextFrame();

		VkResult res = vkAcquireNextImageKHR(m_device, m_swapChain, UINT64_MAX, m_beforeUsageSemaphores.getSemaphoreForThisFrame(), VK_NULL_HANDLE, &m_currentImageIndex);
		checkForVkError(res);
	}
	VkSemaphore SwapChainVk::getBeforeUsageSemaphoreCurrentFrame() const
	{
		return m_beforeUsageSemaphores.getSemaphoreForThisFrame();
	}
	VkSemaphore SwapChainVk::getAfterUsageSemaphoreCurrentFrame() const
	{
		return m_afterUsageSemaphores.getSemaphoreForThisFrame();
	}
	TextureHandle SwapChainVk::getTextureHandleCurrentFrame() const
	{
		return m_textureHandles[m_currentImageIndex];
	}

	void SwapChainVk::present(VkQueue presentQueue)
	{
		VkSemaphore waitBeforePresentSem = getAfterUsageSemaphoreCurrentFrame();
		VkPresentInfoKHR presentInfo{};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.pNext = nullptr;
		presentInfo.pSwapchains = &m_swapChain;
		presentInfo.swapchainCount = 1;
		presentInfo.pImageIndices = &m_currentImageIndex;
		presentInfo.pResults = nullptr;
		presentInfo.pWaitSemaphores = &waitBeforePresentSem;
		presentInfo.waitSemaphoreCount = 1;

		checkForVkError(vkQueuePresentKHR(presentQueue, &presentInfo));
	}
}