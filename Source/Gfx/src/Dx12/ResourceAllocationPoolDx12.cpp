#include <Gfx/Dx12/ResourceAllocationPoolDx12.h>
#include <Gfx/GfxTypes.h>
#include <Gfx/Dx12/Dx12CommonIncludes.h>
#include <Gfx/Dx12/Dx12MiscUtils.h>
#include <Common/CommonUtilities.h>
#include <Common/CommonWindowsUtility.h>

namespace YAPT
{



	ResourceAllocationPoolDx12::ResourceAllocationPoolDx12(ResourceManagerDx12& mngr, TextureHandle* textures, size_t textureCount, BufferHandle* buffers, size_t bufferCount, D3D12_HEAP_TYPE heapType)
		:m_resourceMngr(mngr)
	{
		std::vector<D3D12_RESOURCE_DESC> resourceDescs;
		std::vector<D3D12_RESOURCE_STATES> resourceInitialStates;
		std::vector<D3D12_CLEAR_VALUE> clearValues;

		D3D12_RESOURCE_STATES defaultState = D3D12_RESOURCE_STATE_COMMON;
		if (heapType == D3D12_HEAP_TYPE_READBACK)
		{
			defaultState = D3D12_RESOURCE_STATE_COPY_DEST;
		}
		else if (heapType == D3D12_HEAP_TYPE_UPLOAD)
		{
			defaultState = D3D12_RESOURCE_STATE_GENERIC_READ;
		}

#ifdef DX12_DEBUGNAMES_ENABLE
		std::wstring wStr;
#endif

		if (textureCount > 0)
		{
			

			resourceDescs.resize(textureCount);
			resourceInitialStates.resize(textureCount);
			clearValues.resize(textureCount, { DXGI_FORMAT_UNKNOWN});

			size_t currentRtTexCount = 0;
			size_t currentNonRtTexCount = 0;

			for (size_t i = 0; i < textureCount; ++i)
			{
				if (textures[i]->textureDesc.Flags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET || textures[i]->textureDesc.Flags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL)
				{
					size_t index = textureCount - 1 - currentRtTexCount++;
					resourceDescs[index] = textures[i]->textureDesc;
					resourceInitialStates[index] = heapType == D3D12_HEAP_TYPE_DEFAULT ? textures[i]->lastSeenState.getStateForSubResource(0) : defaultState;
					if (textures[i]->clearValue.Format != DXGI_FORMAT_UNKNOWN)
					{
						clearValues[index] = textures[i]->clearValue;
					}
				}
				else
				{
					size_t index = currentNonRtTexCount++;
					resourceDescs[index] = textures[i]->textureDesc;
					resourceInitialStates[index] = heapType == D3D12_HEAP_TYPE_DEFAULT ? textures[i]->lastSeenState.getStateForSubResource(0) : defaultState;
					
				}
			}

			if (currentRtTexCount > 0)
			{
				D3D12_RESOURCE_ALLOCATION_INFO resourceAllocationInfoTextures = m_resourceMngr.getDevice().GetResourceAllocationInfo(0, (UINT)currentRtTexCount, resourceDescs.data() + currentNonRtTexCount);
				m_textureRtHeap = m_resourceMngr.createResourceHeap(heapType, resourceAllocationInfoTextures.SizeInBytes, D3D12_HEAP_FLAG_ALLOW_ONLY_RT_DS_TEXTURES, resourceAllocationInfoTextures.Alignment);
			}
			
			if (currentNonRtTexCount > 0)
			{
				D3D12_RESOURCE_ALLOCATION_INFO resourceAllocationInfoTextures = m_resourceMngr.getDevice().GetResourceAllocationInfo(0, (UINT)currentNonRtTexCount, resourceDescs.data());
				m_textureNonRtHeap = m_resourceMngr.createResourceHeap(heapType, resourceAllocationInfoTextures.SizeInBytes, D3D12_HEAP_FLAG_ALLOW_ONLY_NON_RT_DS_TEXTURES, resourceAllocationInfoTextures.Alignment);
			}
			
			//non Rt textures
			size_t currentOffset = 0;
			for (size_t i = 0; i < currentNonRtTexCount; ++i)
			{
				D3D12_RESOURCE_ALLOCATION_INFO textureAllocationInfo = m_resourceMngr.getDevice().GetResourceAllocationInfo(0, 1, &resourceDescs[i]);
				currentOffset = align(currentOffset, textureAllocationInfo.Alignment);

				ID3D12Resource* resource;
				m_resourceMngr.getDevice().CreatePlacedResource(m_textureNonRtHeap.get(), currentOffset, &resourceDescs[i], resourceInitialStates[i], clearValues[i].Format != DXGI_FORMAT_UNKNOWN ? &clearValues[i] : NULL, IID_PPV_ARGS(&resource));
				textures[i]->resource = resource;
				textures[i]->heapType = heapType;

#ifdef DX12_DEBUGNAMES_ENABLE
				if (!textures[i]->name.empty())
				{
					stringToWString(textures[i]->name, wStr);
					resource->SetName(wStr.c_str());
				}
#endif
				//texture is holding the reference, decrease the original increment from create
				resource->Release();

				currentOffset += textureAllocationInfo.SizeInBytes;
			}

			//Rt textures
			currentOffset = 0;
			for (size_t i = 0; i <  currentRtTexCount; ++i)
			{
				size_t descIndex = textureCount - 1 - i;
				size_t texIndex = i + currentNonRtTexCount;

				D3D12_RESOURCE_ALLOCATION_INFO textureAllocationInfo = m_resourceMngr.getDevice().GetResourceAllocationInfo(0, 1, &resourceDescs[descIndex]);
				currentOffset = align(currentOffset, textureAllocationInfo.Alignment);



				ID3D12Resource* resource;
				m_resourceMngr.getDevice().CreatePlacedResource(m_textureRtHeap.get(), currentOffset, &resourceDescs[descIndex], resourceInitialStates[descIndex], clearValues[descIndex].Format != DXGI_FORMAT_UNKNOWN ? &clearValues[descIndex] : NULL, IID_PPV_ARGS(&resource));
				textures[texIndex]->resource = resource;
				textures[texIndex]->heapType = heapType;

#ifdef DX12_DEBUGNAMES_ENABLE
				if (!textures[i]->name.empty())
				{
					stringToWString(textures[i]->name, wStr);
					resource->SetName(wStr.c_str());
				}
#endif

				//texture is holding the reference, decrease the original increment from create
				resource->Release();

				currentOffset += textureAllocationInfo.SizeInBytes;
			}
		}

		resourceDescs.clear();
		resourceInitialStates.clear();

		if (bufferCount > 0)
		{
			resourceDescs.resize(bufferCount);
			resourceInitialStates.resize(bufferCount);

			for (size_t i = 0; i < bufferCount; ++i)
			{
				resourceDescs[i] = buffers[i]->bufferDesc;
				resourceInitialStates[i] = heapType == D3D12_HEAP_TYPE_DEFAULT ? buffers[i]->lastSeenState.getStateForSubResource(0) : defaultState;
			}


			D3D12_RESOURCE_ALLOCATION_INFO resourceAllocationInfoBuffers = m_resourceMngr.getDevice().GetResourceAllocationInfo(0, (UINT)resourceDescs.size(), resourceDescs.data());

			m_bufferHeap = m_resourceMngr.createResourceHeap(heapType, resourceAllocationInfoBuffers.SizeInBytes, D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS, resourceAllocationInfoBuffers.Alignment);

			size_t currentOffset = 0;
			for (size_t i = 0; i < bufferCount; ++i)
			{
				D3D12_RESOURCE_ALLOCATION_INFO bufferAllocationInfo = m_resourceMngr.getDevice().GetResourceAllocationInfo(0, 1, &resourceDescs[i]);
				currentOffset = align(currentOffset, bufferAllocationInfo.Alignment);
				resourceDescs[i].Width = bufferAllocationInfo.SizeInBytes;

				ID3D12Resource* resource;
				m_resourceMngr.getDevice().CreatePlacedResource(m_bufferHeap.get(), currentOffset, &resourceDescs[i], resourceInitialStates[i], NULL, IID_PPV_ARGS(&resource));
				buffers[i]->resource = resource;
				buffers[i]->heapType = heapType;

#ifdef DX12_DEBUGNAMES_ENABLE
				if (!buffers[i]->name.empty())
				{
					stringToWString(buffers[i]->name, wStr);
					resource->SetName(wStr.c_str());
				}
				
#endif

				//buffer is holding the reference, decrease the original increment from create
				resource->Release();

				currentOffset += bufferAllocationInfo.SizeInBytes;
			}
		}



	}
	ResourceAllocationPoolDx12::~ResourceAllocationPoolDx12()
	{
		if (m_textureNonRtHeap)
		{
			m_resourceMngr.addToPendingDestructionList(m_textureNonRtHeap);
		}

		if (m_textureRtHeap)
		{
			m_resourceMngr.addToPendingDestructionList(m_textureRtHeap);
		}

		if (m_bufferHeap)
		{
			m_resourceMngr.addToPendingDestructionList(m_bufferHeap);
		}
	}

}


