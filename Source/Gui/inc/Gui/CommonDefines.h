#pragma once
#include <Common/Common.h>

#ifdef GUI_MODULE
#define GUI_MODULE_INTERFACE DLL_EXPORT
#else
#define GUI_MODULE_INTERFACE DLL_IMPORT
#endif
