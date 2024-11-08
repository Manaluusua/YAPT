#pragma once

#include <Renderer/Vk/CommonVk.h>

#define LOAD_INSTANCE_PROC_TO_VAR(var, name) var = reinterpret_cast<PFN_##name>(vkGetInstanceProcAddr(instance, #name))
#define LOAD_INSTANCE_PROC(name) LOAD_INSTANCE_PROC_TO_VAR(name, name)
#define LOAD_DEVICE_PROC_TO_VAR(var, name) var = reinterpret_cast<PFN_##name>(vkGetDeviceProcAddr(device, #name))
#define LOAD_DEVICE_PROC(name) LOAD_DEVICE_PROC_TO_VAR(name, name)

namespace YAPT
{
	struct VkExtensions
	{
		void loadExtensionFuncsInstance(VkInstance instance)
		{
			LOAD_INSTANCE_PROC_TO_VAR(vkCreateDebugReportCallbackEXT, vkCreateDebugReportCallbackEXT);
			LOAD_INSTANCE_PROC_TO_VAR(vkDestroyDebugReportCallbackEXT, vkDestroyDebugReportCallbackEXT);
		}

		void loadExtensionFuncsDevice(VkDevice device)
		{
			LOAD_DEVICE_PROC(vkCreateRayTracingPipelinesKHR);
			LOAD_DEVICE_PROC(vkCmdTraceRaysKHR);
			LOAD_DEVICE_PROC(vkCmdTraceRaysIndirectKHR);
			LOAD_DEVICE_PROC(vkGetRayTracingShaderGroupStackSizeKHR);
			LOAD_DEVICE_PROC(vkCmdSetRayTracingPipelineStackSizeKHR);
			LOAD_DEVICE_PROC(vkGetRayTracingShaderGroupHandlesKHR);
			LOAD_DEVICE_PROC(vkGetAccelerationStructureBuildSizesKHR);
			LOAD_DEVICE_PROC(vkCreateAccelerationStructureKHR);
			LOAD_DEVICE_PROC(vkDestroyAccelerationStructureKHR);
			LOAD_DEVICE_PROC(vkCmdBuildAccelerationStructuresKHR);
		}


		//extension functions. For now use expose here, could move these somewhere else or or vk loader 
		PFN_vkCreateRayTracingPipelinesKHR vkCreateRayTracingPipelinesKHR;
		PFN_vkCmdTraceRaysKHR vkCmdTraceRaysKHR;
		PFN_vkCmdTraceRaysIndirectKHR vkCmdTraceRaysIndirectKHR;
		PFN_vkGetRayTracingShaderGroupStackSizeKHR vkGetRayTracingShaderGroupStackSizeKHR;
		PFN_vkCmdSetRayTracingPipelineStackSizeKHR vkCmdSetRayTracingPipelineStackSizeKHR;
		PFN_vkGetRayTracingShaderGroupHandlesKHR vkGetRayTracingShaderGroupHandlesKHR;
		PFN_vkGetAccelerationStructureBuildSizesKHR vkGetAccelerationStructureBuildSizesKHR;
		PFN_vkCreateAccelerationStructureKHR vkCreateAccelerationStructureKHR;
		PFN_vkDestroyAccelerationStructureKHR vkDestroyAccelerationStructureKHR;
		PFN_vkCmdBuildAccelerationStructuresKHR vkCmdBuildAccelerationStructuresKHR;

		//dbg
		PFN_vkCreateDebugReportCallbackEXT vkCreateDebugReportCallbackEXT = NULL;
		PFN_vkDestroyDebugReportCallbackEXT vkDestroyDebugReportCallbackEXT = NULL;
	};
}