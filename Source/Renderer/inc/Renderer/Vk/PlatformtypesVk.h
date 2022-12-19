#pragma once

#include <Renderer/Vk/CommonVk.h>

#include <vector>
#include <string>

namespace YAPT
{
	class RendererVk;
	class ComputePipelineStateVk;
	class CommandBufferPoolVk;
	class DescriptorSetLayoutVk;
	class DescriptorSetPoolVk;
	class DescriptorSetVk;
	class SwapChainVk;
	struct ShaderModuleVk;
	struct TextureHandleVk;
	struct BufferHandleVk;
	struct BufferViewVk;

	typedef void* BottomLevelAccelerationStructureHandle;
	typedef void* TopLevelAccelerationStructureHandle;
	typedef RendererVk* GfxApiHandle;
	typedef TextureHandleVk* TextureHandle;
	typedef BufferHandleVk* BufferHandle;
	typedef VkImageView TextureViewHandle;
	typedef BufferViewVk* BufferViewHandle;
	typedef void* ResourceAllocationPoolHandle;
	typedef SwapChainVk* SwapChainHandle;
	typedef CommandBufferPoolVk* CommandBufferPoolHandle;
	typedef VkCommandBuffer CommandBufferHandle;
	typedef void* GraphicsPipelineStateHandle;
	typedef VkPipeline ComputePipelineStateHandle;
	typedef VkPipeline RaytracePipelineStateHandle;
	typedef void* RenderPassHandle;
	typedef DescriptorSetLayoutVk* DescriptorSetLayoutHandle;
	typedef VkPipelineLayout PipelineLayoutHandle;
	typedef ShaderModuleVk* ShaderModuleHandle;
	typedef VkSampler SamplerHandle;
	typedef DescriptorSetPoolVk* DescriptorSetPoolHandle;
	typedef DescriptorSetVk* DescriptorSetHandle;
	typedef void* ShaderTableHandle;

	struct QueueDefinitionVk
	{
		VkQueue queue;
		uint32_t queueFamilyIndex;
		uint32_t queueIndex;
		VkQueueFamilyProperties props;
	};
}
