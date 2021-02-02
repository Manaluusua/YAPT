#pragma once

#include <Renderer/Vk/CommonVk.h>

#include <vector>
#include <string>

namespace YAPT
{
	class RendererVk;
	struct TextureHandleVk;
	struct BufferHandleVk;
	struct BufferViewVk;


	struct SamplerHandleVk
	{
		VkSampler sampler;
	};

	typedef void* BottomLevelAccelerationStructureHandle;
	typedef void* TopLevelAccelerationStructureHandle;
	typedef RendererVk* GfxApiHandle;
	typedef TextureHandleVk* TextureHandle;
	typedef BufferHandleVk* BufferHandle;
	typedef VkImageView TextureViewHandle;
	typedef BufferViewVk* BufferViewHandle;
	typedef void* ResourceAllocationPoolHandle;
	typedef void* SwapChainHandle;
	typedef void* CommandBufferPoolHandle;
	typedef void* CommandBufferHandle;
	typedef void* GraphicsPipelineStateHandle;
	typedef void* ComputePipelineStateHandle;
	typedef void* RaytracePipelineStateHandle;
	typedef void* RenderPassHandle;
	typedef void* DescriptorSetLayoutHandle;
	typedef void* PipelineLayoutHandle;
	typedef void* ShaderModuleHandle;
	typedef SamplerHandleVk* SamplerHandle;
	typedef void* DescriptorSetPoolHandle;
	typedef void* DescriptorSetHandle;
	typedef void* ShaderTableHandle;

	struct QueueDefinitionVk
	{
		VkQueue queue;
		uint32_t queueFamilyIndex;
		uint32_t queueIndex;
		VkQueueFamilyProperties props;
	};
}
