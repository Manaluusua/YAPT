#include <Renderer/Shared/GfxApi.h>
#include <Renderer/Vk/RendererVk.h>
#include <Renderer/VK/ResourceManagerVk.h>
#include <Renderer/Vk/ResourceAllocationPoolVk.h>
#include <Renderer/Vk/YaptToVkConversions.h>
#include <Renderer/Vk/ResourceHandlesVk.h>
#include <Renderer/Vk/ShaderPipelineReflectionVk.h>
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
			assert(!"NOT IMPLEMENTED!");
			return nullptr;
		}

		void destroySwapChain(GfxApiHandle h, SwapChainHandle sw)
		{
			assert(!"NOT IMPLEMENTED!");
		}

		void present(GfxApiHandle h, SwapChainHandle swapChain)
		{
			assert(!"NOT IMPLEMENTED!");
		}

		TextureHandle getTextureHandleToNextBackbuffer(SwapChainHandle swapChain)
		{
			assert(!"NOT IMPLEMENTED!");
			return nullptr;
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


		ResourceAllocationPoolHandle createResourcePool(GfxApiHandle h, TextureHandle* textures, size_t textureCount, BufferHandle* buffers, size_t bufferCount, ResourcePoolType type)
		{
			ResourceAllocationPoolVk* pool = new ResourceAllocationPoolVk(*h->getResourceManager());
			if (!pool->allocate(textures, textureCount, buffers, bufferCount, type))
			{
				assert(!"failed to allocate memory.");
				delete pool;
				pool = nullptr;
			}

			return pool;

		}
		void destroyResourcePool(GfxApiHandle h, ResourceAllocationPoolHandle handle)
		{
			delete handle;
		}

		TextureHandle createTexture(GfxApiHandle h, const YAPT::TextureDesc& desc, const ResourceStateDescription& initialState, const char* name)
		{
			TextureHandleVk* texHandle = new TextureHandleVk(*h->getResourceManager());
			yaptTextureDescToVk(desc, texHandle->createInfo);
			texHandle->dimensions = desc.dimension;
			texHandle->currentLayout = VK_IMAGE_LAYOUT_UNDEFINED;
#ifdef VK_DEBUGNAMES_ENABLE
			if (name)
			{
				texHandle->name = std::string(name);
			}
#endif
			VkResult res = vkCreateImage(h->getDevice(), &texHandle->createInfo, VK_ALLOC_CB, &texHandle->image);
			checkVkResult(res);

			if (res != VK_SUCCESS)
			{
				delete texHandle;
				texHandle = nullptr;
			}

			return texHandle;
		}
		BufferHandle createBuffer(GfxApiHandle h, const YAPT::BufferDesc& desc, const ResourceStateDescription& initialState, const char* name)
		{
			BufferHandleVk* buffHandle = new BufferHandleVk(*h->getResourceManager());
			yaptBufferDescToVk(desc, buffHandle->createInfo);
#ifdef VK_DEBUGNAMES_ENABLE
			if (name)
			{
				buffHandle->name = std::string(name);
			}
#endif
			VkResult res = vkCreateBuffer(h->getDevice(), &buffHandle->createInfo, VK_ALLOC_CB, &buffHandle->buffer);
			checkVkResult(res);

			if (res != VK_SUCCESS)
			{
				delete buffHandle;
				buffHandle = nullptr;
			}

			return buffHandle;
		}

		void destroyTexture(GfxApiHandle h, TextureHandle handle)
		{
			h->getResourceManager()->deferredDestroyVkResource(handle->image);
			delete handle;
		}
		void destroyBuffer(GfxApiHandle h, BufferHandle handle)
		{
			h->getResourceManager()->deferredDestroyVkResource(handle->buffer);
			delete handle;
		}

		void uploadBuffer(GfxApiHandle h, BufferHandle handle, size_t offsetInBytes, size_t sizeInBytes, const void* data, GpuUploadStage heapType)
		{
			h->getResourceManager()->upload(handle, offsetInBytes, sizeInBytes, data, heapType);
			
		}

		void* mapBuffer(GfxApiHandle h, BufferHandle handle, size_t offsetInBytes, size_t sizeInBytes, GpuUploadStage heapType)
		{
			return h->getResourceManager()->map(handle, offsetInBytes, sizeInBytes, heapType);
		}

		void unmapBuffer(GfxApiHandle h, BufferHandle handle)
		{
			h->getResourceManager()->unmap(handle);
		}

		void uploadTexture(GfxApiHandle h, TextureHandle image, uint32_t arraySliceOffset, uint32_t arraySliceCount, uint32_t mipOffset, uint32_t mipCount, const TextureDataDefinition* textureDataDefinitions, GpuUploadStage heapType)
		{
			h->getResourceManager()->upload(image, arraySliceOffset, arraySliceCount, mipOffset, mipCount, textureDataDefinitions, heapType);
		}

		size_t getBufferMinimumAlignment(GfxApiHandle h, ResourceUsage resourceUsage)
		{
			
			const VkPhysicalDeviceProperties& props = h->getDeviceProps();
			const VkDeviceSize minStorageAlignment = props.limits.minStorageBufferOffsetAlignment;
			const VkDeviceSize minUniformAlignment = props.limits.minUniformBufferOffsetAlignment;

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
				d.dimensions = texHandle->dimensions;
			}

			if (d.arraySliceCount == YAPT_TEXTURE_VIEW_DESC_ALL_SLICES)
			{
				d.arraySliceCount = texHandle->createInfo.arrayLayers;
			}
			if (d.mipCount == YAPT_TEXTURE_VIEW_DESC_ALL_MIPS)
			{
				d.mipCount = texHandle->createInfo.mipLevels;
			}


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


		BufferViewHandle getBufferView(GfxApiHandle h, TopLevelAccelerationStructureHandle handle)
		{
			assert(!"NOT IMPLEMENTED!");
			return nullptr;
		}
		 
		void allocateBottomLevelAccelerationStructures(GfxApiHandle h, const BottomLevelAccelerationStructureDefinition* definitions, size_t numberOfDefinitions, BottomLevelAccelerationStructureHandle* blasArrayOut)
		{
			assert(!"NOT IMPLEMENTED!");
		}
		void buildBottomLevelAccelerationStructures(GfxApiHandle h, CommandBufferHandle buff, const BottomLevelAccelerationStructureDefinition* definitions, size_t numberOfDefinitions, BottomLevelAccelerationStructureHandle* blasArray)
		{
			assert(!"NOT IMPLEMENTED!");
		}
		void destroyBottomLevelAccelerationStructures(GfxApiHandle h, BottomLevelAccelerationStructureHandle* structures, size_t numberOfStructures)
		{
			assert(!"NOT IMPLEMENTED!");
		}

		void allocateTopLevelAccelerationStructures(GfxApiHandle h, const TopLevelAccelerationStructureDefinition* definitions, size_t numberOfDefinitions, TopLevelAccelerationStructureHandle* tlasArray)
		{
			assert(!"NOT IMPLEMENTED!");
		}
		void buildTopLevelAccelerationStructures(GfxApiHandle h, CommandBufferHandle buff, const TopLevelAccelerationStructureDefinition* definitions, size_t numberOfDefinitions, TopLevelAccelerationStructureHandle* tlasArray)
		{
			assert(!"NOT IMPLEMENTED!");
		}
		void destroyTopLevelAccelerationStructures(GfxApiHandle h, TopLevelAccelerationStructureHandle* structures, size_t numberOfStructures)
		{
			assert(!"NOT IMPLEMENTED!");
		}


		DescriptorSetLayoutHandle createDescriptorSetLayout(GfxApiHandle h, const DescriptorSetLayoutBinding* bindings, size_t numberOfBindings, DescriptorSetLayoutFlags flags)
		{
			VkDescriptorSetLayout layout = VK_NULL_HANDLE;
			VkDescriptorSetLayoutCreateInfo info;
			std::vector<VkDescriptorSetLayoutBinding> vkBindings;

			std::unordered_map<VkDescriptorType, uint32_t> countPerType;

			vkBindings.resize(numberOfBindings);
			info.pNext = nullptr;
			info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
			info.flags = (flags & DESCRIPTORSETLAYOUTFLAG_BINDINGS_MAY_ALIAS) != 0 ? VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT : 0;
			info.bindingCount = (uint32_t)vkBindings.size();
			info.pBindings = vkBindings.data();
			
			for (size_t i = 0; i < numberOfBindings; ++i)
			{
				VkDescriptorSetLayoutBinding& vkBinding = vkBindings[i];
				const DescriptorSetLayoutBinding& binding = bindings[i];

				vkBinding.binding = binding.bindingIndex;
				vkBinding.descriptorCount = binding.descriptorCount;
				vkBinding.stageFlags = yaptShaderStagesToVk(binding.shaderStages);
				vkBinding.descriptorType = yaptDescriptorTypeToVk(binding.type);
				vkBinding.pImmutableSamplers = binding.staticSamplers;

				countPerType[vkBinding.descriptorType] += vkBinding.descriptorCount;
			}

			
			checkVkResult(vkCreateDescriptorSetLayout(h->getDevice(), &info, VK_ALLOC_CB, &layout));

			DescriptorSetLayoutHandleVk* handle = new DescriptorSetLayoutHandleVk;
			handle->layout = layout;
			for (auto iter = countPerType.begin(); iter != countPerType.end(); ++iter)
			{
				handle->requiredDescriptorSpacePerType.push_back({ iter->first, iter->second });
			}
			handle->flags = flags;
			return handle;
		}
		void destroyDescriptorSetLayout(GfxApiHandle h, DescriptorSetLayoutHandle layout)
		{
			h->getResourceManager()->deferredDestroyVkResource(layout->layout);
			delete layout;
		}

		PipelineLayoutHandle createPipelineLayout(GfxApiHandle h, const DescriptorSetLayoutHandle* descSetLayouts, size_t numberOfDescriptorSetLayouts)
		{
			assert(!"NOT IMPLEMENTED!");
			return nullptr;
		}
		void destroyPipelineLayout(GfxApiHandle h, PipelineLayoutHandle layout)
		{
			assert(!"NOT IMPLEMENTED!");
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


		CommandBufferPoolHandle createCommandBufferPool(GfxApiHandle h, size_t numberOfBuffersPerFrame, size_t queueId, const char* name)
		{
			assert(!"NOT IMPLEMENTED!");
			return nullptr;
		}
		void destroyCommandBufferPool(GfxApiHandle h, CommandBufferPoolHandle pool)
		{
			assert(!"NOT IMPLEMENTED!");
		}

		CommandBufferHandle startRecording(GfxApiHandle h, CommandBufferPoolHandle grp, size_t bufferIndex)
		{
			assert(!"NOT IMPLEMENTED!");
			return nullptr;
		}
		void stopRecording(GfxApiHandle h, CommandBufferHandle buff)
		{
			assert(!"NOT IMPLEMENTED!");
		}

		void submitCommandBuffers(GfxApiHandle h, CommandBufferHandle* buffers, size_t numberOfBuffers)
		{
			assert(!"NOT IMPLEMENTED!");
		}

		ComputePipelineStateHandle createComputePipelineState(GfxApiHandle h, const ComputePipelineStateDesc& desc)
		{
			assert(!"NOT IMPLEMENTED!");
			return nullptr;
		}
		void destroyComputePipelineState(GfxApiHandle h, ComputePipelineStateHandle state)
		{
			assert(!"NOT IMPLEMENTED!");
		}

		GraphicsPipelineStateHandle createGraphicsPipelineState(GfxApiHandle h, const GraphicsPipelineStateDesc& desc)
		{
			assert(!"NOT IMPLEMENTED!");
			return nullptr;
		}
		void destroyGraphicsPipelineState(GfxApiHandle h, GraphicsPipelineStateHandle state)
		{
			assert(!"NOT IMPLEMENTED!");
		}

		RaytracePipelineStateHandle createRaytracePipelineState(GfxApiHandle h, const RaytracePipelineStateDesc& desc)
		{
			assert(!"NOT IMPLEMENTED!");
			return nullptr;
		}
		void destroyRaytracePipelineState(GfxApiHandle h, RaytracePipelineStateHandle state)
		{
			assert(!"NOT IMPLEMENTED!");
		}

		DescriptorSetPoolHandle createDescriptorSetPool(GfxApiHandle h, DescriptorSetLayoutHandle layout, size_t numberOfDescriptorSets)
		{
			std::vector<VkDescriptorPoolSize> poolSizes;
			poolSizes.reserve(layout->requiredDescriptorSpacePerType.size());

			VkDescriptorPoolCreateInfo poolDef;
			poolDef.pNext = nullptr;
			poolDef.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
			poolDef.maxSets = (uint32_t)numberOfDescriptorSets;
			poolDef.flags = (layout->flags & DESCRIPTORSETLAYOUTFLAG_BINDINGS_MAY_ALIAS) != 0 ? VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT : 0;
			poolDef.pPoolSizes = poolSizes.data();
			poolDef.poolSizeCount = (uint32_t)poolSizes.size();
			VkDescriptorPool pool;
			vkCreateDescriptorPool(h->getDevice(), &poolDef, VK_ALLOC_CB, &pool);

			DescriptorSetPoolVk* p = new DescriptorSetPoolVk;
			p->pool = pool;
			p->sets.resize(numberOfDescriptorSets);

			std::vector<VkDescriptorSet> descSets;
			descSets.resize(numberOfDescriptorSets);

			std::vector<VkDescriptorSetLayout> layoutArray;
			layoutArray.resize(numberOfDescriptorSets, layout->layout);

			VkDescriptorSetAllocateInfo allocInfo;
			allocInfo.pNext = nullptr;
			allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
			allocInfo.descriptorSetCount = (uint32_t)numberOfDescriptorSets;
			allocInfo.descriptorPool = pool;
			allocInfo.pSetLayouts = layoutArray.data();

			vkAllocateDescriptorSets(h->getDevice(), &allocInfo, descSets.data());

			for (size_t i = 0; i < numberOfDescriptorSets; ++i)
			{
				p->sets[i].frameLastUsed = uint32_t(-1);
				p->sets[i].renderer = h;
				p->sets[i].set = descSets[i];
				p->sets[i].inUse = false;
			}

			return p;
		}
		void destroyDescriptorSetPool(GfxApiHandle h, DescriptorSetPoolHandle pool)
		{
			h->getResourceManager()->deferredDestroyVkResource(pool->pool);
			delete pool;
		}


		DescriptorSetHandle getDescriptorSet(DescriptorSetPoolHandle pool, size_t descSetIndex)
		{
			assert(descSetIndex < pool->sets.size());
			return &pool->sets[descSetIndex];
		}

		void useDescriptorSet(DescriptorSetHandle handle)
		{
			handle->inUse = true;
		}

		void freeDescriptorSet(DescriptorSetHandle handle)
		{
			handle->frameLastUsed = handle->renderer->getFrameNumber();
			handle->inUse = false;
		}
		bool isDescriptorSetUnused(DescriptorSetHandle handle)
		{
			if (handle->inUse) return false;

			return (handle->frameLastUsed + handle->renderer->getFramePipelineLength()) < handle->renderer->getFrameNumber();
		}
		 
		 
		void updateDescriptorSet(GfxApiHandle h, DescriptorSetHandle handle, const DescriptorSetUpdate* updates, size_t updateCount)
		{
			assert(!"NOT IMPLEMENTED!");
		}

		void setVertexBuffers(GfxApiHandle h, CommandBufferHandle buff, BufferViewHandle* vertexBuffers, size_t numberOfBuffers, size_t bindingPointOffset)
		{
			assert(!"NOT IMPLEMENTED!");

		}
		void setIndexBuffer(GfxApiHandle h, CommandBufferHandle buff, BufferViewHandle indexBuffer)
		{
			assert(!"NOT IMPLEMENTED!");
		}

		ShaderTableHandle createShaderTable(GfxApiHandle h, RaytracePipelineStateHandle pso, size_t numberOfRayGenShaders, size_t numberOfMissShaders, size_t numberOfHitGroups)
		{
			assert(!"NOT IMPLEMENTED!");
			return nullptr;
		}
		void destroyShaderTable(GfxApiHandle h, ShaderTableHandle shaderTable)
		{
			assert(!"NOT IMPLEMENTED!");
		}


		void setShaderTableEntries(GfxApiHandle h, ShaderTableHandle shaderTable,
			const ShaderTableEntry* rayGenBindings, size_t numberOfRayGenBindings,
			const ShaderTableEntry* missBindings, size_t numberOfMissBindings,
			const ShaderTableEntry* hitGroupBindings, size_t numberOfHitGroupBindings)
		{
			assert(!"NOT IMPLEMENTED!");
		}
		RenderGraph* createRenderGraph(GfxApiHandle h)
		{
			assert(!"NOT IMPLEMENTED!");
			return nullptr;
		}

		void destroyRenderGraph(GfxApiHandle h, RenderGraph* graph)
		{
			assert(!"NOT IMPLEMENTED!");
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
			assert(!"NOT IMPLEMENTED!");
		}
		void setComputePipelineState(GfxApiHandle h, CommandBufferHandle buff, ComputePipelineStateHandle pso)
		{
			assert(!"NOT IMPLEMENTED!");
		}
		void setRaytracePipelineState(GfxApiHandle h, CommandBufferHandle buff, RaytracePipelineStateHandle pso)
		{
			assert(!"NOT IMPLEMENTED!");
		}

		void bindDescriptorSets(GfxApiHandle h, CommandBufferHandle buff, const DescriptorSetBinding* binding, size_t numberOfBindings, size_t* dynamicOffsets, size_t numberOfDynamicOffsets)
		{
			assert(!"NOT IMPLEMENTED!");
		}

		void dispatch(GfxApiHandle h, CommandBufferHandle buff, uint32_t x, uint32_t y, uint32_t z)
		{
			assert(!"NOT IMPLEMENTED!");
		}
		void dispatchRays(GfxApiHandle h, CommandBufferHandle buff, uint32_t x, uint32_t y, uint32_t z, ShaderTableHandle shaderTable)
		{
			assert(!"NOT IMPLEMENTED!");
		
		}
		void draw(GfxApiHandle h, CommandBufferHandle buff, uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance)
		{
			assert(!"NOT IMPLEMENTED!");
		}

	}
}

