#include <Gfx/GfxApi.h>
#include <Gfx/Vk/RendererVk.h>
#include <Gfx/VK/ResourceManagerVk.h>
#include <Gfx/Vk/YaptToVkConversions.h>
#include <Gfx/Vk/ResourceHandlesVk.h>
#include <Gfx/Vk/ShaderPipelineReflectionVk.h>
#include <Gfx/Vk/RenderGraphVk.h>
#include <Gfx/Vk/ComputePipelineStateVk.h>
#include <Gfx/Vk/GraphicsPipelineStateVk.h>
#include <Gfx/Vk/RaytracePipelineStateVk.h>
#include <Gfx/Vk/CommandBufferPoolVk.h>
#include <Gfx/Vk/DescriptorSetLayoutVk.h>
#include <Gfx/Vk/DescriptorSetPoolVk.h>
#include <Gfx/Vk/DescriptorSetVk.h>
#include <Gfx/Vk/SwapChainVk.h>
#include <Gfx/Vk/ShaderTableVk.h>
#include <Gfx/Vk/AccelerationStructureBuilderVk.h>
#include <Math/Math.h>
#include <unordered_map>

namespace YAPT
{
	namespace Gfx
	{
		GfxApiHandle createGfxApiHandle(const GfxApiInitConfig& config)
		{
			RendererVk* renderer = new RendererVk(config);
			if (!renderer->initialize())
			{
				delete renderer;
				renderer = YAPT_NULL_HANDLE;
			}
			return renderer;
		}
		void destroyGfxApiHandle(GfxApiHandle h)
		{
			delete h;
		}

		SwapChainHandle createSwapChain(GfxApiHandle h, const WindowSurfaceDefinition& windowSurface)
		{
			return h->createSwapChain(windowSurface);
		}

		ResourceFormat getSwapChainImageFormat(SwapChainHandle swapChain)
		{
			return swapChain->getSelectedFormat();
		}

		void destroySwapChain(GfxApiHandle h, SwapChainHandle sw)
		{
			h->destroySwapChain(sw);
		}

		void present(GfxApiHandle h, SwapChainHandle swapChain)
		{
			h->present(swapChain);
		}

		TextureHandle getTextureHandleToNextBackbuffer(SwapChainHandle swapChain)
		{
			swapChain->acquireNextFrame();
			return swapChain->getTextureHandleCurrentFrame();
		}

		void prepare(GfxApiHandle h)
		{
			h->prepare();
		}

		void renderBegin(GfxApiHandle h)
		{
			h->renderBegin();
		}


		void executeBegin(GfxApiHandle h)
		{
			h->executeBegin();
		}

		void executeEnd(GfxApiHandle h)
		{
			h->executeEnd();
		}

		void waitForDeviceIdle(GfxApiHandle h)
		{
			h->waitForAllFramesDone();
		}

		//Resources

		TextureHandle createTexture(GfxApiHandle h, const TextureDesc& desc, const ResourceStateDescription& initialState, const char* name)
		{
			return h->getResourceManager()->createTexture(desc, initialState, h->getGraphicsQueue().queueFamilyIndex, name);
		}
		BufferHandle createBuffer(GfxApiHandle h, const BufferDesc& desc, const ResourceStateDescription& initialState, const char* name)
		{
			return h->getResourceManager()->createBuffer(desc, initialState, h->getGraphicsQueue().queueFamilyIndex, name);
		}

		const TextureDesc& getDesc(GfxApiHandle h, TextureHandle handle)
		{
			return handle->texDesc;
		}
		const BufferDesc& getDesc(GfxApiHandle h, BufferHandle handle)
		{
			return handle->buffDesc;
		}

		void destroyTexture(GfxApiHandle h, TextureHandle handle)
		{
			h->getResourceManager()->destroyTexture(handle);
		}
		void destroyBuffer(GfxApiHandle h, BufferHandle handle)
		{
			h->getResourceManager()->destroyBuffer(handle);
		}

		void uploadBuffer(GfxApiHandle h, BufferHandle handle, size_t offsetInBytes, size_t sizeInBytes, const void* data, GpuUploadStage heapType)
		{
			h->getResourceManager()->upload(handle, offsetInBytes, sizeInBytes, data, heapType);
			
		}

		void* map(GfxApiHandle h, BufferHandle handle, size_t offsetInBytes, size_t sizeInBytes)
		{
			return h->getResourceManager()->map(handle, offsetInBytes, sizeInBytes);
		}
		void unmap(GfxApiHandle h, BufferHandle handle, size_t offsetInBytes, size_t sizeInBytes)
		{
			h->getResourceManager()->unmap(handle, offsetInBytes, sizeInBytes);
		}
		void* map(GfxApiHandle h, TextureHandle handle, uint32_t arraySliceOffset, uint32_t mipOffset)
		{
			return h->getResourceManager()->map(handle, arraySliceOffset, mipOffset);
		}
		void unmap(GfxApiHandle h, TextureHandle handle, uint32_t arraySliceOffset, uint32_t mipOffset)
		{
			h->getResourceManager()->unmap(handle, arraySliceOffset, mipOffset);
		}
		TextureSubresourceLayout getMappedSubresourceLayout(GfxApiHandle h, TextureHandle handle, uint32_t arraySliceOffset, uint32_t mipOffset)
		{
			return h->getResourceManager()->getMappedSubresourceLayout(handle, arraySliceOffset, mipOffset);
		}

		void* mapFromUploadStage(GfxApiHandle h, BufferHandle handle, size_t offsetInBytes, size_t sizeInBytes, GpuUploadStage heapType)
		{
			return h->getResourceManager()->mapUpload(handle, offsetInBytes, sizeInBytes, heapType);
		}

		void unmapFromUploadStage(GfxApiHandle h, BufferHandle handle)
		{
			h->getResourceManager()->unmapUpload(handle);
		}

		void uploadTexture(GfxApiHandle h, TextureHandle image, uint32_t arraySliceOffset, uint32_t arraySliceCount, uint32_t mipOffset, uint32_t mipCount, const TextureDataDefinition* textureDataDefinitions, GpuUploadStage heapType, const ResourceStateDescription& afterUploadUsage)
		{
			h->getResourceManager()->upload(image, arraySliceOffset, arraySliceCount, mipOffset, mipCount, textureDataDefinitions, heapType, afterUploadUsage);
		}

		FenceHandle acquireFence(GfxApiHandle h, FenceType type)
		{
			return h->getResourceManager()->acquireFence(type);
		}

		FenceState getFenceState(GfxApiHandle h, FenceHandle handle)
		{
			return h->getResourceManager()->getFenceState(handle);
		}
		void freeFence(GfxApiHandle h, FenceHandle handle)
		{
			h->getResourceManager()->freeFence(handle);
		}

		void readback(GfxApiHandle h, ReadbackDefinitions& def, GpuDownloadStage stage, FenceHandle fenceToSignal)
		{
			h->getResourceManager()->readback(def, stage, fenceToSignal);
		}

		size_t getBufferMinimumAlignment(GfxApiHandle h, ResourceUsage resourceUsage)
		{
			
			const VkPhysicalDeviceProperties2& props = h->getPhysicalDeviceInfo().deviceProperties2;
			const VkDeviceSize minStorageAlignment = props.properties.limits.minStorageBufferOffsetAlignment;
			const VkDeviceSize minUniformAlignment = props.properties.limits.minUniformBufferOffsetAlignment;

			VkDeviceSize alignment = 1;

			if ((resourceUsage & ResourceUsageBits::RESOURCE_USAGE_UNIFORM_BUFFER) != 0 ||
				(resourceUsage & ResourceUsageBits::RESOURCE_USAGE_UNIFORM_TEXEL_BUFFER) != 0)
			{
				alignment = max(alignment, minUniformAlignment);
			}

			if ((resourceUsage & ResourceUsageBits::RESOURCE_USAGE_STORAGE_BUFFER) != 0 ||
				(resourceUsage & ResourceUsageBits::RESOURCE_USAGE_STORAGE_TEXEL_BUFFER) != 0)
			{
				alignment = max(alignment, minStorageAlignment);
			}


			return alignment;
		}

		TextureViewHandle getTextureView(GfxApiHandle h, TextureHandle texHandle, const TextureViewDesc& desc)
		{
			TextureViewDesc d = desc;

			if (d.dimensions == ResourceDimension::UNDEFINED)
			{
				d.dimensions = texHandle->texDesc.dimension;
			}

			if (d.arraySliceCount == YAPT_TEXTURE_VIEW_DESC_ALL_SLICES)
			{
				d.arraySliceCount = texHandle->createInfo.arrayLayers;
			}
			if (d.mipCount == YAPT_TEXTURE_VIEW_DESC_ALL_MIPS)
			{
				d.mipCount = texHandle->createInfo.mipLevels;
			}

			assert(desc.format != ResourceFormat::UNKNOWN);


			return texHandle->views.get(d);
		}

		BufferViewHandle getBufferView(GfxApiHandle h, BufferHandle bufHandle, const BufferViewDesc& desc)
		{
			BufferViewDesc d = desc;
			if (d.sizeInBytes == YAPT_BUFFER_WHOLE_RESOURCE)
			{
				d.sizeInBytes = bufHandle->createInfo.size;
			}

			if (d.structureStrideInBytes == YAPT_BUFFER_WHOLE_RESOURCE)
			{
				d.structureStrideInBytes = bufHandle->createInfo.size;
			}

			return bufHandle->views.get(d);
		}

		 
		void allocateBottomLevelAccelerationStructures(GfxApiHandle h, const BottomLevelAccelerationStructureDefinition* definitions, size_t numberOfDefinitions, BottomLevelAccelerationStructureHandle* blasArrayOut)
		{
			h->getResourceManager()->getAccelerationStructureBuilder()->allocateBottomLevelAccelerationStructures(definitions, blasArrayOut, numberOfDefinitions);
		}
		void buildBottomLevelAccelerationStructures(GfxApiHandle h, CommandBufferHandle buff,  BottomLevelAccelerationStructureHandle* blasArray, size_t numberOfStructures)
		{
			h->getResourceManager()->getAccelerationStructureBuilder()->buildBottomLevelAccelerationStructures(buff, blasArray, numberOfStructures);
		}
		void destroyBottomLevelAccelerationStructures(GfxApiHandle h, BottomLevelAccelerationStructureHandle* structures, size_t numberOfStructures)
		{
			h->getResourceManager()->getAccelerationStructureBuilder()->destroyBottomLevelAccelerationStructures(structures, numberOfStructures);
		}

		void allocateTopLevelAccelerationStructures(GfxApiHandle h, const TopLevelAccelerationStructureDefinition* definitions, size_t numberOfDefinitions, TopLevelAccelerationStructureHandle* tlasArray)
		{
			h->getResourceManager()->getAccelerationStructureBuilder()->allocateTopLevelAccelerationStructures(definitions, tlasArray, numberOfDefinitions);
		}
		void buildTopLevelAccelerationStructures(GfxApiHandle h, CommandBufferHandle buff, TopLevelAccelerationStructureHandle* tlasArray, size_t numberOfStructures)
		{
			h->getResourceManager()->getAccelerationStructureBuilder()->buildTopLevelAccelerationStructures(buff, tlasArray, numberOfStructures);
		}
		void destroyTopLevelAccelerationStructures(GfxApiHandle h, TopLevelAccelerationStructureHandle* structures, size_t numberOfStructures)
		{
			h->getResourceManager()->getAccelerationStructureBuilder()->destroyTopLevelAccelerationStructures(structures, numberOfStructures);
		}


		DescriptorSetLayoutHandle createDescriptorSetLayout(GfxApiHandle h, const DescriptorSetLayoutBinding* bindings, size_t numberOfBindings, DescriptorSetLayoutFlags flags)
		{
			return  DescriptorSetLayoutVk::create(h, bindings, numberOfBindings, flags);
		}
		void destroyDescriptorSetLayout(GfxApiHandle h, DescriptorSetLayoutHandle layout)
		{
			delete layout;
		}

		PipelineLayoutHandle createPipelineLayout(GfxApiHandle h, const DescriptorSetLayoutHandle* descSetLayouts, size_t numberOfDescriptorSetLayouts)
		{
			VkPipelineLayoutCreateInfo info{};
			info.pNext = nullptr;
			info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
			info.flags = 0;

			info.pPushConstantRanges = nullptr;
			info.pushConstantRangeCount = 0;

			std::vector<VkDescriptorSetLayout> layouts(numberOfDescriptorSetLayouts);
			for (size_t i = 0; i < numberOfDescriptorSetLayouts; ++i)
			{
				layouts[i] = descSetLayouts[i]->getLayout();
			}

			info.setLayoutCount = (uint32_t)numberOfDescriptorSetLayouts;
			info.pSetLayouts = layouts.data();

			VkPipelineLayout layout;
			checkVkResult(vkCreatePipelineLayout(h->getDevice(), &info, VK_ALLOC_CB, &layout));
			return layout;
		}
		void destroyPipelineLayout(GfxApiHandle h, PipelineLayoutHandle layout)
		{
			vkDestroyPipelineLayout(h->getDevice(), layout, VK_ALLOC_CB);
		}

		SamplerHandle createSampler(GfxApiHandle h, const SamplerDescription& desc)
		{
			VkSampler handle = VK_NULL_HANDLE;
			VkSamplerCreateInfo info;
			yaptSamplerDescToVk(desc, info);
			VkResult res = vkCreateSampler(h->getDevice(), &info, VK_ALLOC_CB, &handle);
			checkVkResult(res);

			return handle;
		}
		void destroySampler(GfxApiHandle h, SamplerHandle sampler)
		{
			h->getResourceManager()->deferredDestroyVkResource(sampler);
		}

		ShaderModuleHandle createShaderModuleFromFile(GfxApiHandle h, const char* filepath, ShaderModuleType moduleType, const char* entryPoint, const ShaderModuleDefine* defines, size_t defineCount)
		{
			return h->getResourceManager()->createShaderModule(filepath, moduleType, entryPoint, defines, defineCount);
		}
		void destroyShaderModule(GfxApiHandle h, ShaderModuleHandle m)
		{
			h->getResourceManager()->destroyShaderModule(m);
		}

		size_t getQueueId(GfxApiHandle h, QueueType type)
		{
			switch (type)
			{
			case YAPT::QueueType::QUEUE_TYPE_GRAPHICS:
				return h->getGraphicsQueue().queueFamilyIndex;
			case YAPT::QueueType::QUEUE_TYPE_COMPUTE:
				return h->getComputeQueue().queueFamilyIndex;
			default:
				return h->getGraphicsQueue().queueFamilyIndex;
			}
			 
		}

		CommandBufferPoolHandle createCommandBufferPool(GfxApiHandle h, size_t numberOfBuffersPerFrame, size_t queueId, const char* name)
		{
			return h->getResourceManager()->createCommandBufferPool(numberOfBuffersPerFrame, queueId);
		}
		void destroyCommandBufferPool(GfxApiHandle h, CommandBufferPoolHandle pool)
		{
			h->getResourceManager()->destroyCommandBufferPool(pool);
		}

		void resetCommandPool(GfxApiHandle h, CommandBufferPoolHandle pool)
		{
			pool->resetPool(h->getFramePipelineIndex());
		}

		CommandBufferHandle startRecording(GfxApiHandle h, CommandBufferPoolHandle grp, size_t bufferIndex)
		{
			return grp->beginCommandBufferRecording(h->getFramePipelineIndex(), bufferIndex);
		}
		void stopRecording(GfxApiHandle h, CommandBufferHandle buff)
		{
			CommandBufferPoolVk::endCommandBufferRecording(buff);
		}

		void submitCommandBuffers(GfxApiHandle h, CommandBufferHandle* buffers, size_t numberOfBuffers)
		{
			h->submitCommandLists(buffers, numberOfBuffers);
		}

		ComputePipelineStateHandle createComputePipelineState(GfxApiHandle h, const ComputePipelineStateDesc& desc)
		{
			return ComputePipelineStateVk::create(h, desc);
		}
		void destroyComputePipelineState(GfxApiHandle h, ComputePipelineStateHandle state)
		{
			ComputePipelineStateVk::destroy(h, state);
		}

		GraphicsPipelineStateHandle createGraphicsPipelineState(GfxApiHandle h, const GraphicsPipelineStateDesc& desc)
		{
			return GraphicsPipelineStateVk::create(h, desc);
			
		}
		void destroyGraphicsPipelineState(GfxApiHandle h, GraphicsPipelineStateHandle state)
		{
			GraphicsPipelineStateVk::destroy(h, state);
		}

		RaytracePipelineStateHandle createRaytracePipelineState(GfxApiHandle h, const RaytracePipelineStateDesc& desc)
		{
			return RaytracePipelineStateVk::create(h, desc);
		}
		void destroyRaytracePipelineState(GfxApiHandle h, RaytracePipelineStateHandle state)
		{
			RaytracePipelineStateVk::destroy(h, state);
		}

		DescriptorSetPoolHandle createDescriptorSetPool(GfxApiHandle h, DescriptorSetLayoutHandle layout, size_t numberOfDescriptorSets)
		{
			return DescriptorSetPoolVk::create(h, layout, numberOfDescriptorSets);
			
		}
		void destroyDescriptorSetPool(GfxApiHandle h, DescriptorSetPoolHandle pool)
		{
			delete pool;
		}


		DescriptorSetHandle getDescriptorSet(DescriptorSetPoolHandle pool, size_t descSetIndex)
		{
			return pool->getDescriptorSet(descSetIndex);
		}

		void useDescriptorSet(DescriptorSetHandle handle)
		{
			handle->setInUse(true);
		}

		void freeDescriptorSet(DescriptorSetHandle handle)
		{
			handle->setInUse(false);
		}
		bool isDescriptorSetUnused(DescriptorSetHandle handle)
		{
			return handle->isInUse();
		}
		 
		 
		void updateDescriptorSet(GfxApiHandle h, DescriptorSetHandle handle, const DescriptorSetUpdate* updates, size_t updateCount)
		{
			handle->updateContents(updates, updateCount);
		}

		void setVertexBuffers(GfxApiHandle h, CommandBufferHandle buff, BufferViewHandle* vertexBuffers, size_t numberOfBuffers, size_t bindingPointOffset)
		{
			const size_t MAX_VERTEX_BUFFERS_TO_BIND = 32;
			VkBuffer buffers[MAX_VERTEX_BUFFERS_TO_BIND];
			VkDeviceSize offsets[MAX_VERTEX_BUFFERS_TO_BIND];

			assert(numberOfBuffers < MAX_VERTEX_BUFFERS_TO_BIND);

			for (size_t i = 0; i < min(numberOfBuffers, MAX_VERTEX_BUFFERS_TO_BIND); ++i)
			{
				buffers[i] = vertexBuffers[i]->buffer;
				offsets[i] = vertexBuffers[i]->bufferDesc.offsetInBytes;
			}


			vkCmdBindVertexBuffers(buff, (uint32_t)bindingPointOffset, (uint32_t)numberOfBuffers, buffers, offsets);

		}
		void setIndexBuffer(GfxApiHandle h, CommandBufferHandle buff, BufferViewHandle indexBuffer)
		{
			vkCmdBindIndexBuffer(buff, indexBuffer->buffer, indexBuffer->bufferDesc.offsetInBytes, indexBuffer->bufferDesc.nonStructuredFormat == ResourceFormat::R32_UINT ? VK_INDEX_TYPE_UINT32 : VK_INDEX_TYPE_UINT16);
		}

		ShaderTableHandle createShaderTable(GfxApiHandle h, RaytracePipelineStateHandle pso, size_t numberOfRayGenShaders, size_t numberOfMissShaders, size_t numberOfHitGroups)
		{
			return ShaderTableVk::createShaderTable(h, pso, numberOfRayGenShaders, numberOfMissShaders, numberOfHitGroups);
		}
		void destroyShaderTable(GfxApiHandle h, ShaderTableHandle shaderTable)
		{
			delete shaderTable;
		}


		void setShaderTableEntries(GfxApiHandle h, ShaderTableHandle shaderTable,
			const ShaderTableEntry* rayGenBindings, size_t numberOfRayGenBindings,
			const ShaderTableEntry* missBindings, size_t numberOfMissBindings,
			const ShaderTableEntry* hitGroupBindings, size_t numberOfHitGroupBindings)
		{
			shaderTable->writeShaderTableEntries(rayGenBindings, numberOfRayGenBindings, missBindings, numberOfMissBindings,
				hitGroupBindings, numberOfHitGroupBindings);
		}
		RenderGraph* createRenderGraph(GfxApiHandle h)
		{
			return new RenderGraphVk(h);
		}

		void destroyRenderGraph(GfxApiHandle h, RenderGraph* graph)
		{
			delete graph;
		}

		ShaderPipelineReflection* createShaderPipelineReflection(ShaderModuleHandle* shaderModules, size_t shaderModuleCount)
		{
			return new ShaderPipelineReflectionVk(shaderModules, shaderModuleCount);
		}
		void destroyShaderPipelineReflection(ShaderPipelineReflection* refl)
		{
			delete refl;
		}


		void setGraphicsPipelineState(GfxApiHandle h, CommandBufferHandle buff, GraphicsPipelineStateHandle pso)
		{
			vkCmdBindPipeline(buff, VK_PIPELINE_BIND_POINT_GRAPHICS, pso);
		}
		void setComputePipelineState(GfxApiHandle h, CommandBufferHandle buff, ComputePipelineStateHandle pso)
		{
			vkCmdBindPipeline(buff, VK_PIPELINE_BIND_POINT_COMPUTE, pso);
		}
		void setRaytracePipelineState(GfxApiHandle h, CommandBufferHandle buff, RaytracePipelineStateHandle pso)
		{
			vkCmdBindPipeline(buff, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, pso->getPipeline());
		}

		void bindDescriptorSets(GfxApiHandle h, CommandBufferHandle buff, BindingPoint bindingPoint, PipelineLayoutHandle layout, const DescriptorSetHandle* bindings, size_t firstBindingOffset, size_t numberOfBindings, size_t* dynamicOffsets, size_t numberOfDynamicOffsets)
		{
			const size_t MAX_DESC_SETS_TO_BIND = 32;
			VkDescriptorSet descSets[MAX_DESC_SETS_TO_BIND];
			uint32_t offsets[MAX_DESC_SETS_TO_BIND];

			for (size_t i = 0; i < numberOfBindings; ++i)
			{
				descSets[i] = bindings[i]->nativeDescriptorSet();
			}

			for (size_t i = 0; i < numberOfDynamicOffsets; ++i)
			{
				offsets[i] = (uint32_t)dynamicOffsets[i];
			}

			vkCmdBindDescriptorSets(buff, yaptBindingPointToVk(bindingPoint), layout, (uint32_t)firstBindingOffset, (uint32_t)numberOfBindings, descSets, (uint32_t)numberOfDynamicOffsets, offsets);
		}

		void dispatch(GfxApiHandle h, CommandBufferHandle buff, uint32_t x, uint32_t y, uint32_t z)
		{
			vkCmdDispatch(buff, x, y, z);
		}
		void dispatchRays(GfxApiHandle h, CommandBufferHandle buff, uint32_t x, uint32_t y, uint32_t z, ShaderTableHandle shaderTable)
		{
			VkStridedDeviceAddressRegionKHR raygen = shaderTable->getRayGenShaderTableRange();
			VkStridedDeviceAddressRegionKHR miss = shaderTable->getRayMissShaderTableRange();
			VkStridedDeviceAddressRegionKHR hit = shaderTable->getRayHitShaderTableRange();
			VkStridedDeviceAddressRegionKHR callable = { 0, 0, 0 };

			h->getResourceManager()->getVkExtFuncs().vkCmdTraceRaysKHR(buff, &raygen, &miss, &hit, &callable, x, y, z);
		
		}
		void drawIndexed(GfxApiHandle h, CommandBufferHandle buff, uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance)
		{
			vkCmdDrawIndexed(buff, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
		}

	}
}

