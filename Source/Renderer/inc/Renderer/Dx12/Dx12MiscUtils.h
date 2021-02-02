#pragma once

#include <d3d12.h>
#include <Renderer/RendererCommonTypes.h>
#include <string>

namespace YAPT
{
	//Resource State Utilities
	constexpr D3D12_RESOURCE_STATES D3D12_READ_ONLY_STATES = 
		D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER |
		D3D12_RESOURCE_STATE_INDEX_BUFFER |
		D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE |
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE |
		D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT |
		D3D12_RESOURCE_STATE_COPY_SOURCE |
		D3D12_RESOURCE_STATE_DEPTH_READ;

	constexpr D3D12_RESOURCE_STATES D3D12_WRITE_ONLY_STATES =
		D3D12_RESOURCE_STATE_COPY_DEST | 
		D3D12_RESOURCE_STATE_RENDER_TARGET | 
		D3D12_RESOURCE_STATE_STREAM_OUT;

	constexpr D3D12_RESOURCE_STATES D3D12_READ_WRITE_STATES =
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS | 
		D3D12_RESOURCE_STATE_DEPTH_WRITE;

	constexpr D3D12_RESOURCE_STATES D3D12_ALL_WRITE_STATES = D3D12_WRITE_ONLY_STATES | D3D12_READ_WRITE_STATES;

	bool d3d12StatePromotableFromCommon(D3D12_RESOURCE_STATES firstStateInExecute, bool isBufferOrSimultanousAccessTexture);

	//String Utilities
	void stringToWString(const std::string& str, std::wstring& out);
	void stringToWString(const char* str, std::wstring& out);

	namespace UploadUtility
	{


		//Resource upload utilities
		struct UploadHeapAllocationInfo
		{
			ID3D12Resource* uploadBuffer;
			char* mappedUploadBufferPtr;
			size_t offset;
		};

		struct BufferDataInfo
		{
			ID3D12Resource* dstBuffer;
			UINT64 dstOffset;
			ID3D12Resource* srcBuffer;
			UINT64 srcOffset;
			UINT64 numBytes;
		};

		struct TextureDataInfo
		{
			D3D12_TEXTURE_COPY_LOCATION dstLocation;
			D3D12_TEXTURE_COPY_LOCATION srcLocation;
			UINT dstX;
			UINT dstY;
			UINT dstZ;
		};


		template<typename AllocFunc>
		bool mapCopyRangeForBufferUpload(ID3D12Device5& device, ID3D12Resource* buffer, size_t offsetInBytes, size_t dataSizeInBytes, AllocFunc&& allocator, BufferDataInfo& copyInfoOut, void*& ptrOut);

		template<typename AllocFunc>
		bool uploadDataForTexture(ID3D12Device5& device, ID3D12Resource* texture, const D3D12_RESOURCE_DESC& resourceDesc, size_t arraySliceOffset, size_t arraySliceCount,
			size_t mipOffset, size_t mipCount, const TextureDataDefinition* textureDataDefinitions, AllocFunc&& allocator, TextureDataInfo* uploadTextureDataArrayOut);

	}




		////////////////////////////template implementations ////////////////////////////////////////////////////////
	namespace UploadUtility
	{
		template<typename AllocFunc>
		bool mapCopyRangeForBufferUpload(ID3D12Device5& device, ID3D12Resource* buffer, size_t offsetInBytes, size_t dataSizeInBytes, AllocFunc&& allocator, BufferDataInfo& copyInfoOut, void*& ptrOut)
		{
			UploadHeapAllocationInfo allocInfo;
			if (!allocator(dataSizeInBytes, allocInfo))
			{
				return false;
			}

			copyInfoOut.dstBuffer = buffer;
			copyInfoOut.dstOffset = offsetInBytes;
			copyInfoOut.srcBuffer = allocInfo.uploadBuffer;
			copyInfoOut.srcOffset = allocInfo.offset;
			copyInfoOut.numBytes = dataSizeInBytes;

			ptrOut = reinterpret_cast<void*>(allocInfo.mappedUploadBufferPtr + allocInfo.offset);

			return true;
		}
		template<typename AllocFunc>
		bool uploadDataForTexture(ID3D12Device5& device, ID3D12Resource* texture, const D3D12_RESOURCE_DESC& resourceDesc, size_t arraySliceOffset, size_t arraySliceCount,
			size_t mipOffset, size_t mipCount, const TextureDataDefinition* textureDataDefinitions, AllocFunc&& allocator, TextureDataInfo* uploadTextureDataArrayOut)
		{

			size_t totalNumberOfSubresources = arraySliceCount * mipCount;

			std::vector<D3D12_PLACED_SUBRESOURCE_FOOTPRINT> layoutsArray(totalNumberOfSubresources);
			std::vector<UINT> numRowsArray(totalNumberOfSubresources);
			std::vector<UINT64> rowSizeInBytesArray(totalNumberOfSubresources);
			std::vector<size_t> perArraySliceOffsets(arraySliceCount);
			UINT64 totalBytes = 0;

			//mips are always in continuous subresource indices so can ask all the mips for a slice at once. In case of resource with mipcount == 1, could copy the whole thing at once. 
			for (size_t i = 0; i < arraySliceCount; ++i)
			{
				perArraySliceOffsets[i] = totalBytes;

				UINT64 sizeOfThisBatch = 0;
				UINT arraySlice = (UINT)(i + arraySliceOffset);
				UINT subResourceOffset = D3D12CalcSubresource((UINT)mipOffset, arraySlice, 0, resourceDesc.MipLevels, (UINT)arraySliceCount);

				size_t infoOffset = i * mipCount;

				device.GetCopyableFootprints(&resourceDesc, subResourceOffset, (UINT)mipCount, 0, &layoutsArray[infoOffset], &numRowsArray[infoOffset], &rowSizeInBytesArray[infoOffset], &sizeOfThisBatch);
				//no need to align the last entry
				if (i != (arraySliceCount - 1))
				{
					sizeOfThisBatch = YAPT::align(sizeOfThisBatch, D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT);
				}

				totalBytes += sizeOfThisBatch;
			}

			//request extra from heap to be able to align the first resource (others are automatically aligned to the boundary)
			totalBytes += D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT;


			UploadHeapAllocationInfo allocInfo;
			if (!allocator(totalBytes, allocInfo))
			{
				return false;
			}

			//need to calculate the actual aligned offset from the uploadheap start since the copy to final texture from upload heap to texture needs the uploadheap resource and offset
			char* uploadDst = allocInfo.mappedUploadBufferPtr;
			char* alignedAndOffsetUploadDestination = uploadDst + allocInfo.offset;
			alignedAndOffsetUploadDestination = (char*)YAPT::align(alignedAndOffsetUploadDestination, D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT);

			UINT64 offsetFromUploadHeapStart = alignedAndOffsetUploadDestination - uploadDst;
			

			for (size_t i = 0; i < arraySliceCount; ++i)
			{

				UINT arrayIndex = (UINT)(arraySliceOffset + i);
				size_t infoOffset = i * mipCount;
				UINT64 baseOffset = layoutsArray[infoOffset].Offset;

				for (size_t k = 0; k < mipCount; ++k)
				{
					size_t footPrintIndex = infoOffset + k;
					UINT mipIndex = (UINT)(mipOffset + k);

					D3D12_PLACED_SUBRESOURCE_FOOTPRINT& footPrint = layoutsArray[footPrintIndex];
					UINT numRows = numRowsArray[footPrintIndex];
					UINT64 rowSizeInBytes = rowSizeInBytesArray[footPrintIndex];

					UINT subResourceIndex = D3D12CalcSubresource(mipIndex, arrayIndex, 0, resourceDesc.MipLevels, (UINT)arraySliceCount);

					//memory offset fixup, the offset is relative to the highest mip, but it doesn't take into account potentially having several slicer so we fix that here. 
					UINT64 memOffset = footPrint.Offset - baseOffset + offsetFromUploadHeapStart + perArraySliceOffsets[i];
					footPrint.Offset = memOffset; 

					D3D12_MEMCPY_DEST memCpyDst;
					memCpyDst.pData = uploadDst + memOffset;
					memCpyDst.RowPitch = footPrint.Footprint.RowPitch;
					memCpyDst.SlicePitch = memCpyDst.RowPitch * numRows;

					D3D12_SUBRESOURCE_DATA memCpySrc;
					memCpySrc.pData = textureDataDefinitions[footPrintIndex].data;

					memCpySrc.RowPitch = textureDataDefinitions[footPrintIndex].rowPitchInBytes;
					memCpySrc.SlicePitch = memCpySrc.RowPitch * numRows;

					UINT numberOfSlices = resourceDesc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE3D ? resourceDesc.DepthOrArraySize : 1;

					MemcpySubresource(&memCpyDst, &memCpySrc, rowSizeInBytes, numRows, numberOfSlices);


					//fill in the upload params to be submitted to cmdlist
					TextureDataInfo& uploadTexInfo = uploadTextureDataArrayOut[footPrintIndex];
					uploadTexInfo.dstX = 0;
					uploadTexInfo.dstY = 0;
					uploadTexInfo.dstZ = 0;
					uploadTexInfo.srcLocation = CD3DX12_TEXTURE_COPY_LOCATION(allocInfo.uploadBuffer, footPrint);
					uploadTexInfo.dstLocation = CD3DX12_TEXTURE_COPY_LOCATION(texture, subResourceIndex);

				}
			}


			return true;
		}
	}
}