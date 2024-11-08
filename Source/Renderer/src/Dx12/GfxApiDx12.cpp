#include <Renderer/Shared/GfxApi.h>
#include <Renderer/Dx12/RendererDx12.h>
#include <Renderer/Dx12/ResourceManagerDx12.h>
#include <Renderer/Dx12/YaptToDx12Conversions.h>
#include <Renderer/Dx12/ResourceAllocationPoolDx12.h>
#include <Renderer/Dx12/AccelerationStructureBuilderDx12.h>
#include <Renderer/Dx12/CommandListPoolerDx12.h>
#include <Renderer/Dx12/RenderGraphDx12.h>
#include <Renderer/Dx12/DescriptorHeapDx12.h>
#include <Renderer/Dx12/DescriptorSetLayoutDx12.h>
#include <Renderer/Dx12/PipelineLayoutDx12.h>
#include <Renderer/Dx12/SwapChainDx12.h>
#include <Renderer/Dx12/ComputePipelineStateDx12.h>
#include <Renderer/Dx12/GraphicsPipelineStateDx12.h>
#include <Renderer/Dx12/RaytracePipelineStateDx12.h>
#include <Renderer/Dx12/ShaderPipelineReflectionDx12.h>
#include <Renderer/Dx12/DescriptorSetPoolDx12.h>
#include <Renderer/Dx12/ShaderTableDx12.h>
#include <Math/Math.h>

namespace YAPT
{
	namespace Gfx
	{
		GfxApiHandle createGfxApiHandle(const GfxApiInitConfig& config)
		{
			RendererDx12* renderer = new RendererDx12(config);
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
			return swapChain->acquireNextBackBuffer();
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
			D3D12_HEAP_TYPE heapType = D3D12_HEAP_TYPE_DEFAULT;
			if (type == ResourcePoolType::RESOURCEPOOL_TYPE_CPU_MAPPABLE_UPLOAD)
			{
				heapType = D3D12_HEAP_TYPE_UPLOAD;
			}
			else if (type == ResourcePoolType::RESOURCEPOOL_TYPE_CPU_MAPPABLE_READBACK)
			{
				heapType = D3D12_HEAP_TYPE_READBACK;
			}
			ResourceAllocationPoolDx12* rc = new ResourceAllocationPoolDx12(h->getResourceManager(), textures, textureCount, buffers, bufferCount, heapType);
			return rc;

		}
		void destroyResourcePool(GfxApiHandle h, ResourceAllocationPoolHandle handle)
		{
			delete handle;
		}

		TextureHandle createTexture(GfxApiHandle h, const YAPT::TextureDesc& desc, const ResourceStateDescription& initialState, const char* name)
		{
			TextureHandleDx12* textureHandle = new TextureHandleDx12;
			textureDescToDx12ResourceDesc(desc, textureHandle->textureDesc);
			textureHandle->dimension = desc.dimension;
			textureHandle->lastSeenState.init(yaptUsageToDx12ResourceStates(initialState.resourceUsage, initialState.accessFlags, initialState.shaderStagesUsedIn), desc.depthOrSlices * desc.mips);

			if (desc.useOptimizedClearValue)
			{
				textureHandle->clearValue.Format = textureHandle->textureDesc.Format;
				if (desc.optimizedClearValue.type == ClearValue::_ClearValueType::DEPTH_STENCIL)
				{
					textureHandle->clearValue.DepthStencil.Depth = desc.optimizedClearValue.value.depthStencil.depth;
					textureHandle->clearValue.DepthStencil.Stencil = desc.optimizedClearValue.value.depthStencil.stencil;
				}
				else
				{
					memcpy(textureHandle->clearValue.Color, desc.optimizedClearValue.value.fvec, sizeof(float)*4);
				}
				
			}
			else
			{
				textureHandle->clearValue.Format = DXGI_FORMAT_UNKNOWN;
			}

#ifdef DX12_DEBUGNAMES_ENABLE
			if (name)
			{
				textureHandle->name = std::string(name);
			}
#endif
			return textureHandle;
		}
		BufferHandle createBuffer(GfxApiHandle h, const YAPT::BufferDesc& desc, const ResourceStateDescription& initialState, const char* name)
		{
			BufferHandleDx12* bufferHandle = new BufferHandleDx12;
			bufferDescToDx12ResourceDesc(desc, bufferHandle->bufferDesc);
			bufferHandle->lastSeenState.init(yaptUsageToDx12ResourceStates(initialState.resourceUsage, initialState.accessFlags, initialState.shaderStagesUsedIn), 1);
#ifdef DX12_DEBUGNAMES_ENABLE
			if (name)
			{
				bufferHandle->name = std::string(name);
			}
			
#endif

			return bufferHandle;
		}

		void destroyTexture(GfxApiHandle h, TextureHandle handle)
		{
			h->getResourceManager().addToPendingDestructionList(handle->resource);
			delete handle;
		}
		void destroyBuffer(GfxApiHandle h, BufferHandle handle)
		{
			h->getResourceManager().addToPendingDestructionList(handle->resource);
			delete handle;
		}

		void uploadBuffer(GfxApiHandle h, BufferHandle handle, size_t offsetInBytes, size_t sizeInBytes, const void* data, GpuUploadStage heapType)
		{
			h->getResourceManager().upload(handle, offsetInBytes, sizeInBytes, data, heapType);
			
		}

		void* mapBuffer(GfxApiHandle h, BufferHandle handle, size_t offsetInBytes, size_t sizeInBytes, GpuUploadStage heapType)
		{
			return h->getResourceManager().map(handle, offsetInBytes, sizeInBytes, heapType);
		}

		void unmapBuffer(GfxApiHandle h, BufferHandle handle)
		{
			h->getResourceManager().unmap(handle);
		}

		void uploadTexture(GfxApiHandle h, TextureHandle image, uint32_t arraySliceOffset, uint32_t arraySliceCount, uint32_t mipOffset, uint32_t mipCount, const TextureDataDefinition* textureDataDefinitions, GpuUploadStage heapType)
		{
			h->getResourceManager().upload(image, arraySliceOffset, arraySliceCount, mipOffset, mipCount, textureDataDefinitions, heapType);
		}

		size_t getBufferMinimumAlignment(GfxApiHandle h, ResourceUsage resourceUsage)
		{
			return h->getResourceManager().getBufferMinimumAlignment(resourceUsage);
			
		}

		TextureViewHandle getTextureView(GfxApiHandle h, TextureHandle texHandle, const TextureViewDesc& desc)
		{
			TextureViewDesc d = desc;

			if (d.dimensions == ResourceDimension::UNDEFINED)
			{
				d.dimensions = texHandle->dimension;
			}

			if (d.arraySliceCount == YAPT_TEXTURE_VIEW_DESC_ALL_SLICES)
			{
				d.arraySliceCount = texHandle->textureDesc.DepthOrArraySize;
			}
			if (d.mipCount == YAPT_TEXTURE_VIEW_DESC_ALL_MIPS)
			{
				d.mipCount = texHandle->textureDesc.MipLevels;
			}

			
			return texHandle->views.get(d);
		}
		BufferViewHandle getBufferView(GfxApiHandle h, BufferHandle bufHandle, const BufferViewDesc& desc)
		{
			BufferViewDesc d = desc;
			if (d.sizeInBytes == YAPT_BUFFER_WHOLE_RESOURCE)
			{
				assert(bufHandle->bufferDesc.Width > desc.offsetInBytes);
				d.sizeInBytes = bufHandle->bufferDesc.Width - desc.offsetInBytes;
			}

			if (d.structureStrideInBytes == YAPT_BUFFER_WHOLE_RESOURCE)
			{
				d.structureStrideInBytes = d.sizeInBytes;
			}
			return bufHandle->views.get(d);
		}


		BufferViewHandle getBufferView(GfxApiHandle h, TopLevelAccelerationStructureHandle handle)
		{
			return &handle->accelerationStructure;
		}

		void allocateBottomLevelAccelerationStructures(GfxApiHandle h, const BottomLevelAccelerationStructureDefinition* definitions, size_t numberOfDefinitions, BottomLevelAccelerationStructureHandle* blasArrayOut)
		{
			h->getAccelerationStructureBuilder().allocateBottomLevelAccelerationStructures(definitions, numberOfDefinitions, blasArrayOut);
		}
		 
		void buildBottomLevelAccelerationStructures(GfxApiHandle h, CommandBufferHandle buff, BottomLevelAccelerationStructureHandle* blasArrayOut, size_t numberOfDefinitions)
		{
			h->getAccelerationStructureBuilder().buildBottomLevelAccelerationStructures(buff->cmdList, blasArrayOut, numberOfDefinitions);
		}
		void destroyBottomLevelAccelerationStructures(GfxApiHandle h, BottomLevelAccelerationStructureHandle* structures, size_t numberOfStructures)
		{
			h->getAccelerationStructureBuilder().destroyBottomLevelAccelerationStructures(structures, numberOfStructures);
		}
		
		

		void allocateTopLevelAccelerationStructures(GfxApiHandle h, const TopLevelAccelerationStructureDefinition* definitions, size_t numberOfDefinitions, TopLevelAccelerationStructureHandle* tlasArray)
		{
			h->getAccelerationStructureBuilder().allocateTopLevelAccelerationStructures(definitions, numberOfDefinitions, tlasArray);
		}

		void buildTopLevelAccelerationStructures(GfxApiHandle h, CommandBufferHandle buff, TopLevelAccelerationStructureHandle* tlasArrayOut, size_t numberOfDefinitions)
		{
			h->getAccelerationStructureBuilder().buildTopLevelAccelerationStructures(buff->cmdList, tlasArrayOut, numberOfDefinitions);
		}
		void destroyTopLevelAccelerationStructures(GfxApiHandle h, TopLevelAccelerationStructureHandle* structures, size_t numberOfStructures)
		{
			h->getAccelerationStructureBuilder().destroyTopLevelAccelerationStructures(structures, numberOfStructures);
		}

		DescriptorSetLayoutHandle createDescriptorSetLayout(GfxApiHandle h, const DescriptorSetLayoutBinding* bindings, size_t numberOfBindings, DescriptorSetLayoutFlags flags)
		{
			return new DescriptorSetLayoutDx12(bindings, numberOfBindings, flags);
		}
		void destroyDescriptorSetLayout(GfxApiHandle h, DescriptorSetLayoutHandle layout)
		{
			static_cast<DescriptorSetLayoutDx12*>(layout)->Release();
		}

		PipelineLayoutHandle createPipelineLayout(GfxApiHandle h, const DescriptorSetLayoutHandle* descSetLayouts, size_t numberOfDescriptorSetLayouts)
		{
			PipelineLayoutDx12* layout = new PipelineLayoutDx12(h, descSetLayouts, numberOfDescriptorSetLayouts);
			return layout;
		}
		void destroyPipelineLayout(GfxApiHandle h, PipelineLayoutHandle layout)
		{
			static_cast<PipelineLayoutDx12*>(layout)->Release();
		}

		SamplerHandle createSampler(GfxApiHandle h, const SamplerDescription& desc)
		{
			SamplerDx12* s = new SamplerDx12;
			yaptSamplerDescriptionToDx12(desc, s->samplerDesc);
			return s;
		}
		void destroySampler(GfxApiHandle h, SamplerHandle sampler)
		{
			delete sampler;
		}

		ShaderModuleHandle createShaderModuleFromFile(GfxApiHandle h, const char* filepath, ShaderModuleType moduleType, const char* entryPoint, const ShaderModuleDefine* defines, size_t defineCount)
		{
			return h->getResourceManager().createShaderModule(filepath, moduleType, entryPoint, defines, defineCount);
		}
		void destroyShaderModule(GfxApiHandle h, ShaderModuleHandle m)
		{
			h->getResourceManager().destroyShaderModule(m);
		}

		size_t getQueueId(GfxApiHandle h, QueueType type)
		{
			return 0;
		}


		CommandBufferPoolHandle createCommandBufferPool(GfxApiHandle h, size_t numberOfBuffersPerFrame, size_t queueId, const char* name)
		{
			CommandListPoolerDx12* pooler = h->createCommandListPooler();
			pooler->initialize(D3D12_COMMAND_LIST_TYPE_DIRECT, numberOfBuffersPerFrame);
			return pooler;
		}
		void destroyCommandBufferPool(GfxApiHandle h, CommandBufferPoolHandle pool)
		{
			h->destroyCommandListPooler(pool);
		}

		CommandBufferHandle startRecording(GfxApiHandle h, CommandBufferPoolHandle grp, size_t bufferIndex)
		{
			RCPtr<ID3D12CommandAllocator>& allocator = grp->getAllocator(bufferIndex);
			RCPtr<GraphicsCommandListDx12>& cmdList = grp->getCommandList(bufferIndex);
			CommandBufferHandle handle = grp->getHandle(bufferIndex);

			checkForDxError(allocator->Reset());
			checkForDxError(cmdList->Reset(allocator.get(), NULL));

			
			ID3D12DescriptorHeap* heaps[] = { h->getResourceManager().getSamplerDescHeap()->getNativeHeap() , h->getResourceManager().getNonSamplerDescHeap()->getNativeHeap() };

			cmdList->SetDescriptorHeaps((UINT)countOf(heaps), heaps);
			

			handle->cmdList = cmdList;
			handle->boundLayout = nullptr;
			handle->boundPSO = nullptr;
			handle->boundPsoType = CommandBufferHandleDx12::None;

			return handle;
		}
		void stopRecording(GfxApiHandle h, CommandBufferHandle buff)
		{
			checkForDxError(buff->cmdList->Close());
		}

		void submitCommandBuffers(GfxApiHandle h, CommandBufferHandle* buffers, size_t numberOfBuffers)
		{
			h->submitCommandLists(buffers, numberOfBuffers);
		}

		ComputePipelineStateHandle createComputePipelineState(GfxApiHandle h, const ComputePipelineStateDesc& desc)
		{
			return new ComputePipelineStateDx12(h->getResourceManager(), desc);
		}
		void destroyComputePipelineState(GfxApiHandle h, ComputePipelineStateHandle state)
		{
			delete state;
		}

		GraphicsPipelineStateHandle createGraphicsPipelineState(GfxApiHandle h, const GraphicsPipelineStateDesc& desc)
		{
			return new GraphicsPipelineStateDx12(h->getResourceManager(), desc);
		}
		void destroyGraphicsPipelineState(GfxApiHandle h, GraphicsPipelineStateHandle state)
		{
			delete state;
		}

		RaytracePipelineStateHandle createRaytracePipelineState(GfxApiHandle h, const RaytracePipelineStateDesc& desc)
		{
			return new RaytracePipelineStateDx12(h->getResourceManager(), desc);
		}
		void destroyRaytracePipelineState(GfxApiHandle h, RaytracePipelineStateHandle state)
		{
			delete state;
		}

		DescriptorSetPoolHandle createDescriptorSetPool(GfxApiHandle h, DescriptorSetLayoutHandle layout, size_t numberOfDescriptorSets)
		{
			return h->getResourceManager().createDescriptorSetPool(layout, numberOfDescriptorSets);
		}
		void destroyDescriptorSetPool(GfxApiHandle h, DescriptorSetPoolHandle pool)
		{
			h->getResourceManager().destroyDescriptorSetPool(pool);
		}


		DescriptorSetHandle getDescriptorSet(DescriptorSetPoolHandle pool, size_t descSetIndex)
		{
			return pool->getDescriptorSet(descSetIndex);
			
		}

		void useDescriptorSet(DescriptorSetHandle handle)
		{
			handle->pool->useDescriptorSet(handle);
		}

		void freeDescriptorSet(DescriptorSetHandle handle)
		{
			handle->pool->freeDescriptorSet(handle);
		}
		bool isDescriptorSetUnused(DescriptorSetHandle handle)
		{
			return handle->pool->isDescriptorSetUnused(handle);
		}
		 
		 
		void updateDescriptorSet(GfxApiHandle h, DescriptorSetHandle handle, const DescriptorSetUpdate* updates, size_t updateCount)
		{
			handle->pool->updateDescriptorSet(handle, updates, updateCount);
		}

		void setVertexBuffers(GfxApiHandle h, CommandBufferHandle buff, BufferViewHandle* vertexBuffers, size_t numberOfBuffers, size_t bindingPointOffset)
		{
			assert(numberOfBuffers < 16);
			D3D12_VERTEX_BUFFER_VIEW vertexBuffersDx12[16];

			assert(buff->boundPsoType == CommandBufferHandleDx12::Graphics);
			GraphicsPipelineStateDx12* graphicsPso = static_cast<GraphicsPipelineStateDx12*>(buff->boundPSO);

			for (UINT i = 0; i < numberOfBuffers; ++i)
			{
				D3D12_VERTEX_BUFFER_VIEW& viewDx12 = vertexBuffersDx12[i];
				BufferViewHandle yaptView = vertexBuffers[i];

				viewDx12.BufferLocation = yaptView->resource->GetGPUVirtualAddress() + yaptView->desc.offsetInBytes;
				viewDx12.SizeInBytes = (UINT)yaptView->desc.sizeInBytes;
				viewDx12.StrideInBytes = (UINT)graphicsPso->getStrideForVertexBuffer(i + bindingPointOffset);
			}

			buff->cmdList->IASetVertexBuffers((UINT)bindingPointOffset, (UINT)numberOfBuffers, vertexBuffersDx12);

		}
		void setIndexBuffer(GfxApiHandle h, CommandBufferHandle buff, BufferViewHandle indexBuffer)
		{
			D3D12_INDEX_BUFFER_VIEW view;
			view.BufferLocation = indexBuffer->resource->GetGPUVirtualAddress() + indexBuffer->desc.offsetInBytes;
			view.Format = yaptToDx12Format(indexBuffer->desc.nonStructuredFormat);
			view.SizeInBytes = (UINT)indexBuffer->desc.sizeInBytes;

			buff->cmdList->IASetIndexBuffer(&view);
		}

		ShaderTableHandle createShaderTable(GfxApiHandle h, RaytracePipelineStateHandle pso, size_t numberOfRayGenShaders, size_t numberOfMissShaders, size_t numberOfHitGroups)
		{
			return new ShaderTableDx12(h->getResourceManager(), pso, numberOfRayGenShaders, numberOfMissShaders, numberOfHitGroups);
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
			shaderTable->commit(rayGenBindings, numberOfRayGenBindings, missBindings, numberOfMissBindings, hitGroupBindings, numberOfHitGroupBindings);
		}
		RenderGraph* createRenderGraph(GfxApiHandle h)
		{
			return new RenderGraphDx12(h);
		}

		void destroyRenderGraph(GfxApiHandle h, RenderGraph* graph)
		{
			delete graph;
		}

		ShaderPipelineReflection* createShaderPipelineReflection(ShaderModuleHandle* shaderModules, size_t shaderModuleCount)
		{
			return new ShaderPipelineReflectionDx12(shaderModules, shaderModuleCount);
		}
		void destroyShaderPipelineReflection(ShaderPipelineReflection* refl)
		{
			delete refl;
		}


		void setGraphicsPipelineState(GfxApiHandle h, CommandBufferHandle buff, GraphicsPipelineStateHandle pso)
		{
			pso->bind(buff);

		}
		void setComputePipelineState(GfxApiHandle h, CommandBufferHandle buff, ComputePipelineStateHandle pso)
		{
			pso->bind(buff);

		}
		void setRaytracePipelineState(GfxApiHandle h, CommandBufferHandle buff, RaytracePipelineStateHandle pso)
		{
			pso->bind(buff);

		}

		void bindDescriptorSets(GfxApiHandle h, CommandBufferHandle buff, BindingPoint bindingPoint, PipelineLayoutHandle layout, const DescriptorSetHandle* bindings, size_t firstBindingOffset, size_t numberOfBindings, size_t* dynamicOffsets, size_t numberOfDynamicOffsets)
		{
			buff->boundLayout->bindRootParameters(buff->cmdList, bindings, firstBindingOffset, numberOfBindings,  dynamicOffsets, numberOfDynamicOffsets, bindingPoint != BindingPoint::BINDING_POINT_GRAPHICS);
		}

		void dispatch(GfxApiHandle h, CommandBufferHandle buff, uint32_t x, uint32_t y, uint32_t z)
		{
			buff->cmdList->Dispatch(x, y, z);
		}
		void dispatchRays(GfxApiHandle h, CommandBufferHandle buff, uint32_t x, uint32_t y, uint32_t z, ShaderTableHandle shaderTable)
		{
			D3D12_DISPATCH_RAYS_DESC rayDesc = {};

			rayDesc.Width = (UINT)x;
			rayDesc.Height = (UINT)y;
			rayDesc.Depth = (UINT)z;
			

			rayDesc.RayGenerationShaderRecord.StartAddress = shaderTable->getRayGenShaderTableGpuBaseAddress();
			rayDesc.RayGenerationShaderRecord.SizeInBytes = shaderTable->getRayGenShaderSectionSizeInBytes();

			rayDesc.MissShaderTable.StartAddress = shaderTable->getMissShaderTableGpuBaseAddress();
			rayDesc.MissShaderTable.SizeInBytes = shaderTable->getMissShaderSectionSizeInBytes();
			rayDesc.MissShaderTable.StrideInBytes = shaderTable->getMissShaderEntryMaxAlignedSizeInBytes();
			
			
			rayDesc.HitGroupTable.StartAddress = shaderTable->getHitGroupShaderTableGpuBaseAddress();
			rayDesc.HitGroupTable.SizeInBytes = shaderTable->getHitGroupSectionSizeInBytes();
			rayDesc.HitGroupTable.StrideInBytes = shaderTable->getHitGroupEntryMaxAlignedSizeInBytes();
			

			buff->cmdList->DispatchRays(&rayDesc);
		
		}
		void drawIndexed(GfxApiHandle h, CommandBufferHandle buff, uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance)
		{
			buff->cmdList->DrawIndexedInstanced(indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
		}

	}
}

