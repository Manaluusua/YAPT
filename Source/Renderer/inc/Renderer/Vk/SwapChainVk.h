#pragma once

#include <Renderer/Shared/GfxTypes.h>


namespace YAPT
{
	VkSurfaceKHR createVkSurface(VkInstance instance, YaptRenderSurfaceHandle handle);
}