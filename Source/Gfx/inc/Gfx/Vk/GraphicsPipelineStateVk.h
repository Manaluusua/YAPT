#pragma once


#include <Gfx/Vk/CommonVk.h>
#include <Gfx/GfxTypes.h>
namespace YAPT
{

	class GraphicsPipelineStateVk
	{
	public:
		static VkPipeline create(RendererVk* renderer, const GraphicsPipelineStateDesc& desc);
		static void destroy(RendererVk* renderer, VkPipeline pipeline);
	};
}