#ifndef YAPT_RESOURCEALLOCATIONPOOL_DX12_H
#define YAPT_RESOURCEALLOCATIONPOOL_DX12_H

#include <Renderer/Shared/GfxTypes.h>
#include <Renderer/Dx12/ResourceManagerDx12.h>
namespace YAPT
{

	class ResourceAllocationPoolDx12 
	{

		friend class TextureDx12;
		friend class BufferDx12;

	public:
		ResourceAllocationPoolDx12(ResourceManagerDx12& mngr, TextureHandle* textures, size_t textureCount, BufferHandle* buffers, size_t bufferCount, D3D12_HEAP_TYPE heapType = D3D12_HEAP_TYPE_DEFAULT);
		virtual ~ResourceAllocationPoolDx12();

	private:
		ResourceManagerDx12& m_resourceMngr;
		RCPtr<ID3D12Heap> m_bufferHeap;
		RCPtr<ID3D12Heap> m_textureNonRtHeap;
		RCPtr<ID3D12Heap> m_textureRtHeap;
	};
}


#endif