#include <Renderer/Vk/SwapChainVk.h>
#include <WinUser.h>
namespace YAPT
{
	VkSurfaceKHR createVkSurface(VkInstance instance, YaptRenderSurfaceHandle handle)
	{
		VkSurfaceKHR surface;
		VkWin32SurfaceCreateInfoKHR info{};
		info.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
		info.hwnd = handle;
		info.hinstance = (HINSTANCE)GetWindowLongPtr(info.hwnd, GWLP_HINSTANCE);
		VkResult res = vkCreateWin32SurfaceKHR(instance, &info, NULL, &surface);
		return surface;
	}
}