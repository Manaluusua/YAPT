#pragma once


#include <Renderer/Vk/CommonVk.h>
#include <Renderer/Shared/GfxTypes.h>
namespace YAPT
{

	class GraphicsPipelineStateVk
	{
	public:
		static VkPipeline create(RendererVk* renderer, const GraphicsPipelineStateDesc& desc);
		static void destroy(RendererVk* renderer, VkPipeline pipeline);
	};
}