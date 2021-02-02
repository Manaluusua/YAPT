#pragma once

#include <Common/Common.h>

#ifdef SCENE_MODULE
#define SCENE_MODULE_INTERFACE DLL_EXPORT
#else
#define SCENE_MODULE_INTERFACE DLL_IMPORT
#endif
