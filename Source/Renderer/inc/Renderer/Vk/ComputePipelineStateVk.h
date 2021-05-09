#pragma once


#include <Renderer/Vk/CommonVk.h>
#include <Renderer/Shared/GfxTypes.h>
namespace YAPT
{

	class ComputePipelineStateVk
	{
	public:
		static VkPipeline create(RendererVk* renderer, const ComputePipelineStateDesc& desc);
		static void destroy(RendererVk* renderer, VkPipeline pipeline);
	};
}