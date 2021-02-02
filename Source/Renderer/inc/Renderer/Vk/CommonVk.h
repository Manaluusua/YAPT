#pragma once
#ifdef YAPT_WINDOWS
	#define VK_USE_PLATFORM_WIN32_KHR //for now we can just assume we are only windows and define this here instead of cmake
#endif

#include <vulkan/vulkan.h>
#include <Common/Logger.h>
#define YAPT_CONSIDER_VK_RESULT_ERROR_FATAL
#define checkForVkError(res) (checkVkResult(res))

#define VK_ALLOC_CB (nullptr)

#define VK_DEBUGNAMES_ENABLE

namespace YAPT
{
	inline bool checkVkResult(VkResult res)
	{
#ifdef YAPT_CONSIDER_VK_RESULT_ERROR_FATAL
		if (res != VK_SUCCESS)
		{
			YAPT_LOG_FATAL_ERROR("Vk Error (%d)", res);
		}
#endif
		return res == VK_SUCCESS;
	}



}