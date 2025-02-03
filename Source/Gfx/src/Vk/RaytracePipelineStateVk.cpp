#include <Gfx/Vk/RaytracePipelineStateVk.h>
#include <Gfx/Vk/RendererVk.h>
#include <Gfx/Vk/ResourceManagerVk.h>
#include <Gfx/Vk/ShaderModuleVk.h>
#include <Gfx/Vk/YaptToVkConversions.h>

namespace YAPT
{
	RaytracePipelineStateVk* RaytracePipelineStateVk::create(RendererVk* renderer, const RaytracePipelineStateDesc& desc)
	{
		VkDevice device = renderer->getDevice();
		const auto& props = renderer->getPhysicalDeviceInfo().raytracePipelineProperties;

		VkPipeline pipeline = VK_NULL_HANDLE;
		VkRayTracingPipelineCreateInfoKHR createInfo{};
		createInfo.pNext = NULL;
		createInfo.sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR;
		createInfo.flags = renderer->isGPUCaptureEnabled() ? VK_PIPELINE_CREATE_RAY_TRACING_SHADER_GROUP_HANDLE_CAPTURE_REPLAY_BIT_KHR : 0; //VK_PIPELINE_CREATE_RAY_TRACING_NO_NULL_ANY_HIT_SHADERS_BIT_KHR 

		createInfo.layout = desc.layout;

		createInfo.basePipelineHandle = VK_NULL_HANDLE;
		createInfo.basePipelineIndex = -1;

		createInfo.pDynamicState = NULL;
		
		createInfo.pLibraryInterface = NULL;
		createInfo.pLibraryInfo = NULL;

		createInfo.maxPipelineRayRecursionDepth = (uint32_t)desc.maxTraceRecursionDepth;

		std::vector<VkRayTracingShaderGroupCreateInfoKHR> groupsArray;
		groupsArray.resize(desc.numberOfHitGroupDescription + desc.numberOfRayGenerationDescription + desc.numberOfRayMissDescription);

		std::vector<VkPipelineShaderStageCreateInfo> stagesArray;
		stagesArray.reserve(desc.numberOfHitGroupDescription * 3 + desc.numberOfRayGenerationDescription + desc.numberOfRayMissDescription);

		auto addShaderStage = [&](size_t indexInModulearray, ShaderStageBits stage) -> uint32_t
		{
			stagesArray.resize(stagesArray.size() + 1);
			VkPipelineShaderStageCreateInfo& shdInfo = stagesArray.back();;
			const ShaderStageCreateInfo& shdInfoSrc = desc.shaders[indexInModulearray];

			shdInfo.pNext = NULL;
			shdInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;

			shdInfo.pSpecializationInfo = NULL;

			shdInfo.module = shdInfoSrc.shaderModule->m_vkShaderModule;
			shdInfo.pName = shdInfoSrc.entryPoint;
			shdInfo.stage = yaptShaderStageBitstoVk(stage);
			shdInfo.flags = 0;

			return (uint32_t)(stagesArray.size() - 1);
		};

		//convert shaders
		/*for (size_t i = 0; i < desc.numberOfShaders; ++i)
		{
			VkPipelineShaderStageCreateInfo& shdInfo = stagesArray[i];
			const ShaderStageCreateInfo& shdInfoSrc = desc.shaders[i];

			shdInfo.pNext = NULL;
			shdInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;

			shdInfo.pSpecializationInfo = NULL;

			shdInfo.module = shdInfoSrc.shaderModule->m_vkShaderModule;
			shdInfo.pName = shdInfoSrc.entryPoint;
			shdInfo.stage = yaptShaderStageBitstoVk(shdInfoSrc.stage);
			shdInfo.flags = 0;
		}*/

		size_t hitGroupOffset;
		size_t missGroupOffset;
		size_t rayGenGroupOffset;

		//convert groups
		size_t groupsArrayIndex = 0;
		hitGroupOffset = groupsArrayIndex;
		for (size_t i = 0; i < desc.numberOfHitGroupDescription; ++i, ++groupsArrayIndex)
		{
			const RayHitGroupDescription& groupDescSrc = desc.hitGroupDescriptions[i];
			VkRayTracingShaderGroupCreateInfoKHR& groupsEntry = groupsArray[groupsArrayIndex];

			groupsEntry.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
			groupsEntry.pNext = NULL;

			groupsEntry.pShaderGroupCaptureReplayHandle = NULL;
			groupsEntry.generalShader = VK_SHADER_UNUSED_KHR;

			groupsEntry.type = groupDescSrc.hitGroupType == HitGroupType::BOUNDINGBOX ? VK_RAY_TRACING_SHADER_GROUP_TYPE_PROCEDURAL_HIT_GROUP_KHR : VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;

			groupsEntry.closestHitShader = groupDescSrc.closestHitShaderIndex == YAPT_NULL_INDEX ? VK_SHADER_UNUSED_KHR : addShaderStage(groupDescSrc.closestHitShaderIndex, ShaderStageBits::SHADERSTAGE_RT_CLOSEST_HIT);
			groupsEntry.intersectionShader = groupDescSrc.intersectionShaderIndex == YAPT_NULL_INDEX ? VK_SHADER_UNUSED_KHR : addShaderStage(groupDescSrc.intersectionShaderIndex, ShaderStageBits::SHADERSTAGE_RT_INTERSECTION);
			groupsEntry.anyHitShader = groupDescSrc.anyHitShaderIndex == YAPT_NULL_INDEX ? VK_SHADER_UNUSED_KHR : addShaderStage(groupDescSrc.anyHitShaderIndex, ShaderStageBits::SHADERSTAGE_RT_ANY_HIT);
		}

		rayGenGroupOffset = groupsArrayIndex;
		for (size_t i = 0; i < desc.numberOfRayGenerationDescription; ++i, ++groupsArrayIndex)
		{
			const RayGenerationDescription& rayGenDesc = desc.rayGenerationDescriptions[i];
			VkRayTracingShaderGroupCreateInfoKHR& groupsEntry = groupsArray[groupsArrayIndex];

			groupsEntry.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
			groupsEntry.pNext = NULL;

			groupsEntry.pShaderGroupCaptureReplayHandle = NULL;
			groupsEntry.closestHitShader = VK_SHADER_UNUSED_KHR;
			groupsEntry.intersectionShader = VK_SHADER_UNUSED_KHR;
			groupsEntry.anyHitShader = VK_SHADER_UNUSED_KHR;

			groupsEntry.generalShader = addShaderStage(rayGenDesc.shaderIndex, ShaderStageBits::SHADERSTAGE_RT_RAYGENERATION);

			groupsEntry.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;

			
		}

		missGroupOffset = groupsArrayIndex;
		for (size_t i = 0; i < desc.numberOfRayMissDescription; ++i, ++groupsArrayIndex)
		{
			const RayMissDescription& missDesc = desc.rayMissDescriptions[i];
			VkRayTracingShaderGroupCreateInfoKHR& groupsEntry = groupsArray[groupsArrayIndex];

			groupsEntry.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
			groupsEntry.pNext = NULL;

			groupsEntry.pShaderGroupCaptureReplayHandle = NULL;
			groupsEntry.closestHitShader = VK_SHADER_UNUSED_KHR;
			groupsEntry.intersectionShader = VK_SHADER_UNUSED_KHR;
			groupsEntry.anyHitShader = VK_SHADER_UNUSED_KHR;

			groupsEntry.generalShader = addShaderStage(missDesc.shaderIndex, ShaderStageBits::SHADERSTAGE_RT_MISS);

			groupsEntry.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
		}

		createInfo.stageCount = (uint32_t)stagesArray.size();
		createInfo.pStages = stagesArray.data();
		createInfo.groupCount = (uint32_t)groupsArray.size();
		createInfo.pGroups = groupsArray.data();


		VkResult res = renderer->getVkExtFuncs().vkCreateRayTracingPipelinesKHR(device, VK_NULL_HANDLE, VK_NULL_HANDLE, 1, &createInfo, VK_ALLOC_CB, &pipeline);
		checkVkResult(res);

		RaytracePipelineStateVk* rtPipeline = new RaytracePipelineStateVk();
		rtPipeline->m_pipeline = pipeline;
		rtPipeline->m_hitGroupConstantsSizeInBytes = desc.hitGroupShaderTableConstantsSizeInBytes;
		rtPipeline->m_missConstantsSizeInBytes = desc.missShaderTableConstantsSizeInBytes;
		rtPipeline->m_rayGenConstantsSizeInBytes = desc.rayGenShaderTableConstantsSizeInBytes;
		rtPipeline->m_hitGroupIndexOffset = hitGroupOffset;
		rtPipeline->m_rayGenIndexOffset = rayGenGroupOffset;
		rtPipeline->m_missGroupIndexOffset = missGroupOffset;


		{
			size_t overallHandleCount = groupsArray.size();
			VkDeviceSize handlesBufferSize = overallHandleCount * props.shaderGroupHandleSize;
			rtPipeline->m_shaderGroupHandles.resize(handlesBufferSize);
			rtPipeline->m_handleSize = props.shaderGroupHandleSize;

			res = renderer->getVkExtFuncs().vkGetRayTracingShaderGroupHandlesKHR(device, pipeline, 0u, (uint32_t)overallHandleCount, handlesBufferSize, rtPipeline->m_shaderGroupHandles.data());
			checkVkResult(res);
		}

		
		return rtPipeline;
	}

	void RaytracePipelineStateVk::destroy(RendererVk* renderer, RaytracePipelineStateVk* pipeline)
	{
		renderer->getResourceManager()->deferredDestroyVkResource(pipeline->getPipeline());
		delete pipeline;
	}
}