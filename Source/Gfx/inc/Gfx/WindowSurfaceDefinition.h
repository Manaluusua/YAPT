#ifndef YAPT_WINDOWSURFACEDEFINITION_H
#define YAPT_WINDOWSURFACEDEFINITION_H

#ifdef YAPT_WINDOWS
#include <Windows.h>
#define YaptRenderSurfaceHandle HWND
#else
#error "Not supported";"
#endif
namespace YAPT
{
	struct WindowSurfaceDefinition
	{
		size_t width;
		size_t height;
		YaptRenderSurfaceHandle windowHandle;
	};
}
#endif

