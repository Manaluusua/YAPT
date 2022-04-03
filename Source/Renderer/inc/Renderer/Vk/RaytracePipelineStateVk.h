#pragma once


#include <Renderer/Vk/CommonVk.h>
#include <Renderer/Shared/GfxTypes.h>
namespace YAPT
{

	class RaytracePipelineStateVk
	{
	public:
		static VkPipeline create(RendererVk* renderer, const RaytracePipelineStateDesc& desc);
		static void destroy(RendererVk* renderer, VkPipeline pipeline);
	};
}