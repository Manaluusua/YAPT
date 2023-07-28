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
	class RaytracePipelineStateVk;
	class ResourceAllocationPoolVk;
	struct ShaderModuleVk;
	struct TextureHandleVk;
	struct BufferHandleVk;
	struct BufferViewVk;
	struct RenderPassHandleVk;

	typedef void* BottomLevelAccelerationStructureHandle;
	typedef void* TopLevelAccelerationStructureHandle;
	typedef RendererVk* GfxApiHandle;
	typedef TextureHandleVk* TextureHandle;
	typedef BufferHandleVk* BufferHandle;
	typedef VkImageView TextureViewHandle;
	typedef BufferViewVk* BufferViewHandle;
	typedef ResourceAllocationPoolVk* ResourceAllocationPoolHandle;
	typedef SwapChainVk* SwapChainHandle;
	typedef CommandBufferPoolVk* CommandBufferPoolHandle;
	typedef VkCommandBuffer CommandBufferHandle;
	typedef VkPipeline GraphicsPipelineStateHandle;
	typedef VkPipeline ComputePipelineStateHandle;
	typedef RaytracePipelineStateVk* RaytracePipelineStateHandle;
	typedef const RenderPassHandleVk* RenderPassHandle;
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

	struct RenderPassHandleVk
	{
		VkRenderPass pass;
		uint32_t index;
	};

	enum CommandQueueType
	{
		COMMANDQUEUETYPE_GRAPHICS = 0,
		COMMANDQUEUETYPE_COMPUTE,
		COMMANDQUEUETYPE_COPY,
		COMMANDQUEUETYPE_COUNT
	};

}
