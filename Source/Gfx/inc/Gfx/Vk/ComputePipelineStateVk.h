#pragma once


#include <Gfx/Vk/CommonVk.h>
#include <Gfx/GfxTypes.h>
namespace YAPT
{

	class ComputePipelineStateVk
	{
	public:
		static VkPipeline create(RendererVk* renderer, const ComputePipelineStateDesc& desc);
		static void destroy(RendererVk* renderer, VkPipeline pipeline);
	};
}