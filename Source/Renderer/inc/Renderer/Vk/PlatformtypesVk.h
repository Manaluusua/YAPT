#pragma once

#include <Renderer/Vk/CommonVk.h>

#include <vector>
#include <string>

namespace YAPT
{
	class RendererVk;
	class ComputePipelineStateVk;
	struct ShaderModuleVk;
	struct TextureHandleVk;
	struct BufferHandleVk;
	struct BufferViewVk;
	
	struct DescriptorSetLayoutHandleVk
	{
		VkDescriptorSetLayout layout;
		std::vector<VkDescriptorPoolSize> requiredDescriptorSpacePerType;
		DescriptorSetLayoutFlags flags;
	};

	struct DescriptorSetVk
	{
		RendererVk* renderer;
		VkDescriptorSet set;
		size_t frameLastUsed;
		bool inUse;
	};

	struct DescriptorSetPoolVk
	{
		VkDescriptorPool pool;
		std::vector<DescriptorSetVk> sets;
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
	typedef VkPipeline ComputePipelineStateHandle;
	typedef void* RaytracePipelineStateHandle;
	typedef void* RenderPassHandle;
	typedef DescriptorSetLayoutHandleVk* DescriptorSetLayoutHandle;
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
