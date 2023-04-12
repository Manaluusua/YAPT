#include <Renderer/Vk/GraphicsPipelineStateVk.h>
#include <Renderer/Vk/RendererVk.h>
#include <Renderer/Vk/ResourceManagerVk.h>
#include <Renderer/Vk/ShaderModuleVk.h>
#include <Renderer/Vk/YaptToVkConversions.h>

namespace YAPT
{
	VkPipeline GraphicsPipelineStateVk::create(RendererVk* renderer, const GraphicsPipelineStateDesc& desc)
	{

		VkPipeline pipeline = VK_NULL_HANDLE;
		VkGraphicsPipelineCreateInfo createInfo{};
		createInfo.pNext = nullptr;
		createInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		createInfo.flags = 0;
		createInfo.basePipelineHandle = VK_NULL_HANDLE;
		createInfo.basePipelineIndex = -1;

		createInfo.layout = desc.pipelineLayout;




		VkResult res = vkCreateGraphicsPipelines(renderer->getDevice(), VK_NULL_HANDLE, 1, &createInfo, VK_ALLOC_CB, &pipeline);
		assert(res == VK_SUCCESS);
		return pipeline;
	}

	void GraphicsPipelineStateVk::destroy(RendererVk* renderer, VkPipeline pipeline)
	{
		renderer->getResourceManager()->deferredDestroyVkResource(pipeline);
	}
}