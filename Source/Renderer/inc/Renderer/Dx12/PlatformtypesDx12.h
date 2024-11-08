#ifndef YAPT_PLATFORMTYPES_DX12_H
#define YAPT_PLATFORMTYPES_DX12_H

#include "Dx12CommonIncludes.h"
#include <Renderer/Shared/Utility/ResourceViewPool.h>
#include <Renderer/Shared/Utility/ResourceStateTracker.h>
#include <dxcapi.h>
#include <vector>
#include <string>

#ifdef ENABLE_DEBUG_UTILITIES_DX12
#define DX12_DEBUGNAMES_ENABLE
#endif

namespace YAPT
{

	class RendererDx12;
	class SwapChainDx12;
	class ResourceAllocationPoolDx12;
	class DescriptorSetLayoutDx12;
	class PipelineLayoutDx12;
	class CommandListPoolerDx12;
	class GraphicsPipelineStateDx12;
	class ComputePipelineStateDx12;
	class RaytracePipelineStateDx12;
	class DescriptorSetPoolDx12;
	class ShaderTableDx12;
	struct BottomLevelAccelerationStructureDx12;
	struct TopLevelAccelerationStructureDx12;
	struct DescriptorSetDx12;
	typedef ResourceStateTracker<D3D12_RESOURCE_STATES> ResourceStateTrackerDx12;

	struct ShaderReflectionDataBindingDx12
	{
		std::string name;
		uint32_t bindPoint;
		uint32_t bindCount;
		uint32_t spaceIndex;
		DescriptorType type;
		AccessFlags accessFlags;
	};

	struct ShaderReflectionDataVertexInputEntryDx12
	{
		AttributeSemantic semantic;
		size_t index;
	};

	struct ShaderReflectionDataDx12
	{
		std::vector<ShaderReflectionDataVertexInputEntryDx12> sortedInputData;
		std::vector<ShaderReflectionDataBindingDx12> sortedBindings;
	};

	struct ShaderModuleDx12
	{
		ShaderModuleType type;
		ShaderReflectionDataDx12 reflection;
		RCPtr<IDxcBlob> shaderBlob;
	};

	struct TextureViewDx12
	{
		TextureViewDesc desc;
		RCPtr<ID3D12Resource> resource;
	};

	struct BufferViewDx12
	{
		BufferViewDesc desc;
		RCPtr<ID3D12Resource> resource;
	};

	

	struct TextureHandleDx12
	{
		typedef ResourceViewPool<TextureViewDesc, TextureViewDx12*, TextureHandleDx12, 3> TextureViews;

		TextureHandleDx12()
			:views(*this)
		{

		}

		TextureViewDx12* createView(const TextureViewDesc& texView)
		{
			TextureViewDx12* tv = new TextureViewDx12;
			tv->desc = texView;
			tv->resource = resource;
			return tv;
		}

		void destroyView(TextureViewDx12* texView)
		{
			delete texView;
		}
		TextureViews views;
		D3D12_RESOURCE_DESC textureDesc;
		ResourceDimension dimension;
		RCPtr<ID3D12Resource> resource;
		D3D12_HEAP_TYPE heapType;
		ResourceStateTrackerDx12 lastSeenState;
		D3D12_CLEAR_VALUE clearValue;

#ifdef DX12_DEBUGNAMES_ENABLE
		std::string name;
#endif
	};

	struct BufferHandleDx12
	{
		typedef ResourceViewPool<BufferViewDesc, BufferViewDx12*, BufferHandleDx12, 3> BufferViews;

		BufferHandleDx12()
			:views(*this),
			mappedBufferPtr(nullptr)
		{

		}

		~BufferHandleDx12()
		{
			if (mappedBufferPtr != nullptr && resource.get())
			{
				resource->Unmap(0, nullptr);
			}
		}

		void* map()
		{
			if (mappedBufferPtr == nullptr && resource.get())
			{
				if (heapType == D3D12_HEAP_TYPE_UPLOAD)
				{
					checkForDxError(resource->Map(0, nullptr, &mappedBufferPtr));
				}
			}

			return mappedBufferPtr;
		}

		void unmap()
		{
			if (mappedBufferPtr != nullptr)
			{
				mappedBufferPtr = nullptr;
				resource->Unmap(0, nullptr);
			}
		}

		BufferViewDx12* createView(const BufferViewDesc& buffView)
		{
			BufferViewDx12* bv = new BufferViewDx12;
			bv->desc = buffView;
			bv->resource = resource;
			return bv;
		}

		void destroyView(BufferViewDx12* buffView)
		{
			delete buffView;
		}
		BufferViews views;
		D3D12_RESOURCE_DESC bufferDesc;
		RCPtr<ID3D12Resource> resource;
		D3D12_HEAP_TYPE heapType;
		ResourceStateTrackerDx12 lastSeenState;


#ifdef DX12_DEBUGNAMES_ENABLE
		std::string name;
#endif
	private:
		void* mappedBufferPtr;

	};


	struct RenderPassDeclarationDx12
	{
		UINT renderTargetCount;
		DXGI_FORMAT rtvFormats[8];
		DXGI_FORMAT dsvFormat;
	};

	struct SamplerDx12
	{
		D3D12_SAMPLER_DESC samplerDesc;
	};

	struct CommandBufferHandleDx12
	{
		GraphicsCommandListDx12* cmdList;
		const PipelineLayoutDx12* boundLayout;
		enum
		{
			None,
			Graphics,
			Compute,
			Raytrace
		} boundPsoType;
		void* boundPSO;
	};


	typedef BottomLevelAccelerationStructureDx12* BottomLevelAccelerationStructureHandle;
	typedef TopLevelAccelerationStructureDx12* TopLevelAccelerationStructureHandle;
	typedef RendererDx12* GfxApiHandle;
	typedef TextureHandleDx12* TextureHandle;
	typedef BufferHandleDx12* BufferHandle;
	typedef TextureViewDx12* TextureViewHandle;
	typedef BufferViewDx12* BufferViewHandle;
	typedef ResourceAllocationPoolDx12* ResourceAllocationPoolHandle;
	typedef SwapChainDx12* SwapChainHandle;
	typedef CommandListPoolerDx12* CommandBufferPoolHandle;
	typedef CommandBufferHandleDx12* CommandBufferHandle;
	typedef GraphicsPipelineStateDx12* GraphicsPipelineStateHandle;
	typedef ComputePipelineStateDx12* ComputePipelineStateHandle;
	typedef RaytracePipelineStateDx12* RaytracePipelineStateHandle;
	typedef RenderPassDeclarationDx12* RenderPassHandle;
	typedef DescriptorSetLayoutDx12* DescriptorSetLayoutHandle;
	typedef PipelineLayoutDx12* PipelineLayoutHandle;
	typedef ShaderModuleDx12* ShaderModuleHandle;
	typedef SamplerDx12* SamplerHandle;
	typedef DescriptorSetPoolDx12* DescriptorSetPoolHandle;
	typedef DescriptorSetDx12* DescriptorSetHandle;
	typedef ShaderTableDx12* ShaderTableHandle;
}

#endif