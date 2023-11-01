#ifndef YAPT_SHARED_GFXAPI_H
#define YAPT_SHARED_GFXAPI_H

#include "GfxTypes.h"

namespace YAPT
{

	namespace Gfx
	{

		
		//Common API calls
		GfxApiHandle createGfxApiHandle(const GfxApiInitConfig& config);
		void destroyGfxApiHandle(GfxApiHandle h);

		void prepare(GfxApiHandle h);
		void renderBegin(GfxApiHandle h);
		void executeBegin(GfxApiHandle h);
		void executeEnd(GfxApiHandle h);

		void waitForDeviceIdle(GfxApiHandle h);

		//swapchain
		SwapChainHandle createSwapChain(GfxApiHandle h, const WindowSurfaceDefinition& windowSurface);
		void destroySwapChain(GfxApiHandle h, SwapChainHandle swapChain);
		TextureHandle getTextureHandleToNextBackbuffer(SwapChainHandle swapChain);
		void present(GfxApiHandle h, SwapChainHandle swapChain);

		//Resources
		ResourceAllocationPoolHandle createResourcePool(GfxApiHandle h, TextureHandle* textures, size_t textureCount, BufferHandle* buffers, size_t bufferCount, ResourcePoolType type);
		void destroyResourcePool(GfxApiHandle h, ResourceAllocationPoolHandle handle);

		TextureHandle createTexture(GfxApiHandle h, const YAPT::TextureDesc& desc, const ResourceStateDescription& initialState, const char* name);
		BufferHandle createBuffer(GfxApiHandle h, const YAPT::BufferDesc& desc, const ResourceStateDescription& initialState, const char* name);
		void destroyTexture(GfxApiHandle h, TextureHandle handle);
		void destroyBuffer(GfxApiHandle h, BufferHandle handle);

		size_t getBufferMinimumAlignment(GfxApiHandle h, ResourceUsage resourceUsage);
		 
		void uploadBuffer(GfxApiHandle h, BufferHandle handle, size_t offsetInBytes, size_t sizeInBytes, const void* data, GpuUploadStage heapType);
		//data is assumed to be: arraySlice * mipCount ie. all mip levels of first array slice, then all mips of second array slice etc.
		void uploadTexture(GfxApiHandle h, TextureHandle image, uint32_t arraySliceOffset, uint32_t arraySliceCount, uint32_t mipOffset, uint32_t mipCount, const TextureDataDefinition* textureDataDefinitions, GpuUploadStage heapType);
		void* mapBuffer(GfxApiHandle h, BufferHandle handle, size_t offsetInBytes, size_t sizeInBytes, GpuUploadStage heapType);
		void unmapBuffer(GfxApiHandle h, BufferHandle handle);

		TextureViewHandle getTextureView(GfxApiHandle h, TextureHandle texHandle, const TextureViewDesc& desc);
		BufferViewHandle getBufferView(GfxApiHandle h, BufferHandle bufHandle, const BufferViewDesc& desc);
		BufferViewHandle getBufferView(GfxApiHandle h, TopLevelAccelerationStructureHandle handle);
		
		void allocateBottomLevelAccelerationStructures(GfxApiHandle h, const BottomLevelAccelerationStructureDefinition* definitions, size_t numberOfDefinitions, BottomLevelAccelerationStructureHandle* blasArrayOut);
		void buildBottomLevelAccelerationStructures(GfxApiHandle h, CommandBufferHandle buff, const BottomLevelAccelerationStructureDefinition* definitions, size_t numberOfDefinitions, BottomLevelAccelerationStructureHandle* blasArray);
		void destroyBottomLevelAccelerationStructures(GfxApiHandle h, BottomLevelAccelerationStructureHandle* structures, size_t numberOfStructures);

		void allocateTopLevelAccelerationStructures(GfxApiHandle h, const TopLevelAccelerationStructureDefinition* definitions, size_t numberOfDefinitions, TopLevelAccelerationStructureHandle* tlasArray);
		void buildTopLevelAccelerationStructures(GfxApiHandle h, CommandBufferHandle buff, const TopLevelAccelerationStructureDefinition* definitions, size_t numberOfDefinitions, TopLevelAccelerationStructureHandle* tlasArray);
		void destroyTopLevelAccelerationStructures(GfxApiHandle h, TopLevelAccelerationStructureHandle* structures, size_t numberOfStructures);

		SamplerHandle createSampler(GfxApiHandle h, const SamplerDescription& desc);
		void destroySampler(GfxApiHandle h, SamplerHandle sampler);

		ShaderModuleHandle createShaderModuleFromFile(GfxApiHandle h, const char* filepath, ShaderModuleType moduleType, const char* entryPoint, const ShaderModuleDefine* defines, size_t defineCount);
		void destroyShaderModule(GfxApiHandle h, ShaderModuleHandle m);

		DescriptorSetLayoutHandle createDescriptorSetLayout(GfxApiHandle h, const DescriptorSetLayoutBinding* bindings, size_t numberOfBindings, DescriptorSetLayoutFlags flags);
		void destroyDescriptorSetLayout(GfxApiHandle h, DescriptorSetLayoutHandle layout);

		PipelineLayoutHandle createPipelineLayout(GfxApiHandle h, const DescriptorSetLayoutHandle* descSetLayouts, size_t numberOfDescriptorSetLayouts);
		void destroyPipelineLayout(GfxApiHandle h, PipelineLayoutHandle layout);

		//commandlists
		size_t getQueueId(GfxApiHandle h, QueueType type);

		CommandBufferPoolHandle createCommandBufferPool(GfxApiHandle h, size_t numberOfBuffersPerFrame, size_t queueId, const char* name);
		void destroyCommandBufferPool(GfxApiHandle h, CommandBufferPoolHandle group);

		CommandBufferHandle startRecording(GfxApiHandle h, CommandBufferPoolHandle pool, size_t bufferIndex);
		void stopRecording(GfxApiHandle h, CommandBufferHandle buff);

		void submitCommandBuffers(GfxApiHandle h, CommandBufferHandle* buffers, size_t numberOfBuffers);

		//psoS
		ComputePipelineStateHandle createComputePipelineState(GfxApiHandle h, const ComputePipelineStateDesc& desc);
		void destroyComputePipelineState(GfxApiHandle h, ComputePipelineStateHandle state);

		GraphicsPipelineStateHandle createGraphicsPipelineState(GfxApiHandle h, const GraphicsPipelineStateDesc& desc);
		void destroyGraphicsPipelineState(GfxApiHandle h, GraphicsPipelineStateHandle state);

		RaytracePipelineStateHandle createRaytracePipelineState(GfxApiHandle h, const RaytracePipelineStateDesc& desc);
		void destroyRaytracePipelineState(GfxApiHandle h, RaytracePipelineStateHandle state);

		DescriptorSetPoolHandle createDescriptorSetPool(GfxApiHandle h, DescriptorSetLayoutHandle layout, size_t numberOfDescriptorSets);
		void destroyDescriptorSetPool(GfxApiHandle h, DescriptorSetPoolHandle pool);

		DescriptorSetHandle getDescriptorSet(DescriptorSetPoolHandle pool, size_t descSetIndex);
		void useDescriptorSet(DescriptorSetHandle handle);
		void freeDescriptorSet(DescriptorSetHandle handle);
		bool isDescriptorSetUnused(DescriptorSetHandle handle);
		
		void updateDescriptorSet(GfxApiHandle h, DescriptorSetHandle handle, const DescriptorSetUpdate* updates, size_t updateCount);

		void setVertexBuffers(GfxApiHandle h, CommandBufferHandle buff, BufferViewHandle* vertexBuffers, size_t numberOfBuffers, size_t bindingPointOffset);
		void setIndexBuffer(GfxApiHandle h, CommandBufferHandle buff, BufferViewHandle indexBuffer);

		ShaderTableHandle createShaderTable(GfxApiHandle h, RaytracePipelineStateHandle pso, size_t numberOfRayGenShaders, size_t numberOfMissShaders, size_t numberOfHitGroups);
		void destroyShaderTable(GfxApiHandle h, ShaderTableHandle shaderTable);
		void setShaderTableEntries(GfxApiHandle h, ShaderTableHandle shaderTable,
			const ShaderTableEntry* rayGenBindings, size_t numberOfRayGenBindings,
			const ShaderTableEntry* missBindings, size_t numberOfMissBindings,
			const ShaderTableEntry* hitGroupBindings, size_t numberOfHitGroupBindings);

		RenderGraph* createRenderGraph(GfxApiHandle h);
		void destroyRenderGraph(GfxApiHandle h, RenderGraph* graph);
		
		ShaderPipelineReflection* createShaderPipelineReflection(ShaderModuleHandle* shaderModules, size_t shaderModuleCount);
		void destroyShaderPipelineReflection(ShaderPipelineReflection* refl);

		//dispatch/draw commands
		void setGraphicsPipelineState(GfxApiHandle h, CommandBufferHandle buff, GraphicsPipelineStateHandle pso);
		void setComputePipelineState(GfxApiHandle h, CommandBufferHandle buff, ComputePipelineStateHandle pso);
		void setRaytracePipelineState(GfxApiHandle h, CommandBufferHandle buff, RaytracePipelineStateHandle pso);

		void bindDescriptorSets(GfxApiHandle h, CommandBufferHandle buff, BindingPoint bindingPoint, PipelineLayoutHandle layout, const DescriptorSetHandle* bindings, size_t firstBindingOffset, size_t numberOfBindings, size_t* dynamicOffsets, size_t numberOfDynamicOffsets);

		void dispatch(GfxApiHandle h, CommandBufferHandle buff, uint32_t x, uint32_t y, uint32_t z);
		void dispatchRays(GfxApiHandle h, CommandBufferHandle buff, uint32_t x, uint32_t y, uint32_t z, ShaderTableHandle shaderTable);
		void drawIndexed(GfxApiHandle h, CommandBufferHandle buff, uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance);

	}
	
}

#endif