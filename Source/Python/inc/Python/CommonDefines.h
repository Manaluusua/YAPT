#pragma once

#include <Common/Common.h>

#ifdef PYTHON_MODULE
#define PYTHON_MODULE_INTERFACE DLL_EXPORT
#else
#define PYTHON_MODULE_INTERFACE DLL_IMPORT
#endif
