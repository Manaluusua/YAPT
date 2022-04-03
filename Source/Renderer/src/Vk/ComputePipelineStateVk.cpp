#include <Renderer/Vk/ComputePipelineStateVk.h>
#include <Renderer/Vk/RendererVk.h>
#include <Renderer/Vk/ResourceManagerVk.h>
#include <Renderer/Vk/ShaderModuleVk.h>
#include <Renderer/Vk/YaptToVkConversions.h>

namespace YAPT
{
	VkPipeline ComputePipelineStateVk::create(RendererVk* renderer, const ComputePipelineStateDesc& desc)
	{
		
		VkPipeline pipeline = VK_NULL_HANDLE;
		VkComputePipelineCreateInfo createInfo{};
		createInfo.pNext = nullptr;
		createInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
		createInfo.flags = 0;
		createInfo.basePipelineHandle = VK_NULL_HANDLE;
		createInfo.basePipelineIndex = -1;

		createInfo.layout = desc.pipelineLayout;

		createInfo.stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		createInfo.stage.pNext = NULL;
		createInfo.stage.flags = 0;
		createInfo.stage.module = desc.shaderStage.shaderModule->m_vkShaderModule;
		createInfo.stage.pName = desc.shaderStage.entryPoint;
		createInfo.stage.stage = yaptShaderStageBitstoVk(desc.shaderStage.stage);
		createInfo.stage.pSpecializationInfo = NULL;



		VkResult res = vkCreateComputePipelines(renderer->getDevice(), VK_NULL_HANDLE, 1, &createInfo, VK_ALLOC_CB, &pipeline);
		assert(res == VK_SUCCESS);
		return pipeline;
	}

	void ComputePipelineStateVk::destroy(RendererVk* renderer, VkPipeline pipeline)
	{
		renderer->getResourceManager()->deferredDestroyVkResource(pipeline);
	}
}