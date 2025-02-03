#include <Gfx/Dx12/ShaderTableDx12.h>
#include <Gfx/Dx12/PipelineLayoutDx12.h>
#include <Gfx/Dx12/ResourceManagerDx12.h>
#include <Gfx/Dx12/ResourceAllocationPoolDx12.h>
#include <Gfx/GfxApi.h>
#include <Gfx/Dx12/YaptToDx12Conversions.h>
#include <Gfx/Dx12/DescriptorSetPoolDx12.h>

namespace YAPT
{
	ShaderTableDx12::ShaderTableDx12(ResourceManagerDx12& resMngr, RaytracePipelineStateDx12* pso, size_t numberOfRayGenShaders, size_t numberOfMissShaders, size_t numberOfHitGroups, bool useSeparateBuffers)
		:m_resMngr(resMngr),
		m_pso(pso),
		m_numberOfRayGenShaders(numberOfRayGenShaders),
		m_numberOfMissShaders(numberOfMissShaders),
		m_numberOfHitGroups(numberOfHitGroups),
		m_rayGenEntrySizeInBytes( (numberOfRayGenShaders > 0) ? RaytracePipelineStateDx12::calculateAlignedShaderRecordSize(pso->getRayGenhaderMaxLocalSignatureSizeInBytes()) : 0),
		m_missEntrySizeInBytes( (numberOfMissShaders > 0) ? RaytracePipelineStateDx12::calculateAlignedShaderRecordSize(pso->getMissShaderMaxLocalSignatureSizeInBytes()) : 0),
		m_hitGroupEntrySizeInBytes((numberOfHitGroups > 0) ? RaytracePipelineStateDx12::calculateAlignedShaderRecordSize(pso->getHitGroupMaxLocalSignatureSizeInBytes()) : 0),
		m_separateBuffersPerType(useSeparateBuffers)
	{
		size_t overallSizeInBytes = getRayGenShaderSectionAlignedSizeInBytes() + getMissShaderSectionAlignedSizeInBytes() + getHitGroupSectionAlignedSizeInBytes();

		size_t bufferCount = m_separateBuffersPerType ? 3 : 1;

		size_t bufferSizes[3] = { m_separateBuffersPerType ? getRayGenShaderSectionAlignedSizeInBytes() : overallSizeInBytes,
		m_separateBuffersPerType ? getMissShaderSectionAlignedSizeInBytes() : 0,
		m_separateBuffersPerType ? getHitGroupSectionAlignedSizeInBytes() : 0 };

		BufferHandleDx12* bufferHandlesToAllocate[3];
		size_t numberOfBufferHandlesToAllocate = 0;

		for (size_t i = 0; i < bufferCount; ++i)
		{
			m_bufferHandles[i] = new BufferHandleDx12;
			m_bufferHandles[i]->lastSeenState.init(D3D12_RESOURCE_STATE_COPY_DEST, 1);
#ifdef DX12_DEBUGNAMES_ENABLE
			m_bufferHandles[i]->name = "ShaderTable";
#endif
			D3D12_RESOURCE_DESC& resourceDesc = m_bufferHandles[i]->bufferDesc;

			resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
			resourceDesc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
			resourceDesc.Width = (UINT64)bufferSizes[i];
			resourceDesc.Height = 1;
			resourceDesc.DepthOrArraySize = 1;
			resourceDesc.MipLevels = 1;
			resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
			resourceDesc.SampleDesc.Count = 1;
			resourceDesc.SampleDesc.Quality = 0;
			resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
			resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

			if (resourceDesc.Width == 0)
			{
				continue;
			}

			bufferHandlesToAllocate[numberOfBufferHandlesToAllocate] = m_bufferHandles[i];
			++numberOfBufferHandlesToAllocate;
		}

		

		m_resourceAllocationPool = new ResourceAllocationPoolDx12(resMngr, nullptr, 0, bufferHandlesToAllocate, numberOfBufferHandlesToAllocate);

		m_shadowBuffer.resize(overallSizeInBytes);

	}


	ShaderTableDx12::~ShaderTableDx12()
	{
		delete m_resourceAllocationPool;
		for (size_t i = 0; i < getActualBufferCount(); ++i)
		{
			delete m_bufferHandles[i];
		}
	}


	size_t ShaderTableDx12::getMissShaderSectionAlignedSizeInBytes() const
	{
		return align(getMissShaderSectionSizeInBytes(), D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT);
	}
	size_t ShaderTableDx12::getRayGenShaderSectionAlignedSizeInBytes() const
	{
		return align(getRayGenShaderSectionSizeInBytes(), D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT);
	}
	size_t ShaderTableDx12::getHitGroupSectionAlignedSizeInBytes() const
	{
		return align(getHitGroupSectionSizeInBytes(), D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT);
	}

	size_t ShaderTableDx12::getRayGenShaderSectionSizeInBytes() const
	{
		return m_rayGenEntrySizeInBytes * m_numberOfRayGenShaders;
	}
	size_t ShaderTableDx12::getMissShaderSectionSizeInBytes() const
	{
		return m_missEntrySizeInBytes * m_numberOfMissShaders;
	}
	size_t ShaderTableDx12::getHitGroupSectionSizeInBytes() const
	{
		return m_hitGroupEntrySizeInBytes * m_numberOfHitGroups;
	}

	size_t ShaderTableDx12::getOffsetToRayGenShaderSectionInBytes() const
	{
		return 0;
	}
	size_t ShaderTableDx12::getOffsetToMissShaderSectionInBytes() const
	{
		return getOffsetToRayGenShaderSectionInBytes() + getRayGenShaderSectionAlignedSizeInBytes();
	}
	size_t ShaderTableDx12::getOffsetToHitGroupSectionInBytes() const
	{
		return getOffsetToMissShaderSectionInBytes() + getMissShaderSectionAlignedSizeInBytes();
	}

	D3D12_GPU_VIRTUAL_ADDRESS ShaderTableDx12::getRayGenShaderTableGpuBaseAddress() const
	{
		if (m_separateBuffersPerType)
		{
			if (m_bufferHandles[RayGenIndex]->bufferDesc.Width > 0)
			{
				return m_bufferHandles[RayGenIndex]->resource->GetGPUVirtualAddress();
			}
			else
			{
				return 0;
			}
		}
		else
		{
			return m_bufferHandles[0]->resource->GetGPUVirtualAddress() + getOffsetToRayGenShaderSectionInBytes();
		}
	}
	D3D12_GPU_VIRTUAL_ADDRESS ShaderTableDx12::getMissShaderTableGpuBaseAddress() const
	{
		if (m_separateBuffersPerType)
		{
			if (m_bufferHandles[MissIndex]->bufferDesc.Width > 0)
			{
				return m_bufferHandles[MissIndex]->resource->GetGPUVirtualAddress();
			}
			else
			{
				return 0;
			}
		}
		else
		{
			return m_bufferHandles[0]->resource->GetGPUVirtualAddress() + getOffsetToMissShaderSectionInBytes();
		}
	}
	D3D12_GPU_VIRTUAL_ADDRESS ShaderTableDx12::getHitGroupShaderTableGpuBaseAddress() const
	{
		if (m_separateBuffersPerType)
		{
			if (m_bufferHandles[HitGroupIndex]->bufferDesc.Width > 0)
			{
				return m_bufferHandles[HitGroupIndex]->resource->GetGPUVirtualAddress();
			}
			else
			{
				return 0;
			}
		}
		else
		{
			return m_bufferHandles[0]->resource->GetGPUVirtualAddress() + getOffsetToHitGroupSectionInBytes();
		}
	}

	void ShaderTableDx12::commit(const ShaderTableEntry* rayGenBindings, size_t numberOfRayGenBindings,
		const ShaderTableEntry* missBindings, size_t numberOfMissBindings,
		const ShaderTableEntry* hitGroupBindings, size_t numberOfHitGroupBindings)
	{
		for (size_t i = 0; i < numberOfRayGenBindings; ++i)
		{
			const ShaderTableEntry& entry = rayGenBindings[i];
			size_t baseOffsetInBytes = getOffsetToRayGenShaderSectionInBytes();
			size_t offsetInBytes = baseOffsetInBytes + entry.shaderTableIndex * getRayGenShaderEntryMaxAlignedSizeInBytes();

			writeShaderEntry(m_shadowBuffer.data() + offsetInBytes, m_pso->getRayGenShaderRecordInfo(entry.shaderIndexInPso), entry);
		}

		for (size_t i = 0; i < numberOfMissBindings; ++i)
		{
			const ShaderTableEntry& entry = missBindings[i];
			size_t baseOffsetInBytes = getOffsetToMissShaderSectionInBytes();
			size_t offsetInBytes = baseOffsetInBytes + entry.shaderTableIndex * getMissShaderEntryMaxAlignedSizeInBytes();

			writeShaderEntry(m_shadowBuffer.data() + offsetInBytes, m_pso->getMissShaderRecordInfo(entry.shaderIndexInPso), entry);
		}

		for (size_t i = 0; i < numberOfHitGroupBindings; ++i)
		{
			const ShaderTableEntry& entry = hitGroupBindings[i];
			size_t baseOffsetInBytes = getOffsetToHitGroupSectionInBytes();
			size_t offsetInBytes = baseOffsetInBytes + entry.shaderTableIndex * getHitGroupEntryMaxAlignedSizeInBytes();

			writeShaderEntry(m_shadowBuffer.data() + offsetInBytes, m_pso->getHitGroupShaderRecordInfo(entry.shaderIndexInPso), entry);
		}

		flushShadowBufferToGPU(numberOfRayGenBindings > 0, numberOfMissBindings > 0, numberOfHitGroupBindings > 0);
	}

	void ShaderTableDx12::flushShadowBufferToGPU(bool rayGenDirty, bool missDirty, bool hitGroupDirty)
	{
		
		
		if (m_separateBuffersPerType)
		{
			if (rayGenDirty)
			{
				size_t dataSize = m_bufferHandles[RayGenIndex]->bufferDesc.Width;
				void* dstPtr = m_resMngr.map(m_bufferHandles[RayGenIndex], 0, dataSize, GpuUploadStage::DURING_RENDER);
				memcpy(dstPtr, m_shadowBuffer.data() + getOffsetToRayGenShaderSectionInBytes(), dataSize);
				m_resMngr.unmap(m_bufferHandles[RayGenIndex]);
			}

			if (missDirty)
			{
				size_t dataSize = m_bufferHandles[MissIndex]->bufferDesc.Width;
				void* dstPtr = m_resMngr.map(m_bufferHandles[MissIndex], 0, dataSize, GpuUploadStage::DURING_RENDER);
				memcpy(dstPtr, m_shadowBuffer.data() + getOffsetToMissShaderSectionInBytes(), dataSize);
				m_resMngr.unmap(m_bufferHandles[RayGenIndex]);
			}

			if (hitGroupDirty)
			{
				size_t dataSize = m_bufferHandles[HitGroupIndex]->bufferDesc.Width;
				void* dstPtr = m_resMngr.map(m_bufferHandles[HitGroupIndex], 0, dataSize, GpuUploadStage::DURING_RENDER);
				memcpy(dstPtr, m_shadowBuffer.data() + getOffsetToHitGroupSectionInBytes(), dataSize);
				m_resMngr.unmap(m_bufferHandles[RayGenIndex]);
			}
			
		} else
		{
			size_t dataSize = m_bufferHandles[0]->bufferDesc.Width;
			void* dstPtr = m_resMngr.map(m_bufferHandles[0], 0, dataSize, GpuUploadStage::DURING_RENDER);
			memcpy(dstPtr, m_shadowBuffer.data(), dataSize);
			m_resMngr.unmap(m_bufferHandles[0]);
		}
		
	}

	void ShaderTableDx12::writeShaderEntry(void* dst, const RaytracePipelineStateDx12::ShaderRecordInfo& record, const ShaderTableEntry& entry)
	{
		char* ptr = static_cast<char*>(dst);
		memcpy(ptr, record.shaderId, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);

		ptr += D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
		memcpy(ptr, entry.shaderTableExtraData, entry.extraDataInBytes);

	}

	/*
	void ShaderTableDx12::writeShaderEntry(void* dst, const RaytracePipelineStateDx12::ShaderRecordInfo& shaderRecordInfo, DescriptorSetBinding* bindings, size_t bindingsCount)
	{
		
		char* ptr = static_cast<char*>(dst);
		memcpy(ptr, &shaderRecordInfo.shaderId, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
		ptr += D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;

		const PipelineLayoutDx12* layout = m_pso->getLocalPipelineLayout(shaderRecordInfo.localRootSigIndex);


		for (size_t i = 0; i < bindingsCount; ++i)
		{
			size_t descSetIndex = bindings[i].descSetIndex;
			DescriptorSetDx12* descSet = bindings[i].descSet;

			const PipelineLayoutDx12::DescriptorSetToRootParametersMapping& mapping = layout->getDescSetToRootParametersMapping(descSetIndex);

			for (size_t i = 0; i < descSet->rootDescriptors.size(); ++i)
			{
				size_t ptrOffset = (mapping.rootDescriptorsRootParameterOffset + i) * sizeof(D3D12_GPU_DESCRIPTOR_HANDLE);
				memcpy(ptr + ptrOffset, &descSet->rootDescriptors[i].resourceAddress, sizeof(D3D12_GPU_DESCRIPTOR_HANDLE));
			}

			if (descSet->nonSamplerHeapIncrementSize > 0)
			{
				size_t ptrOffset = mapping.descriptorTableRootParameterOffsetNonSampler * sizeof(D3D12_GPU_DESCRIPTOR_HANDLE);
				memcpy(ptr + ptrOffset, &descSet->nonSamplerHeapDescSetBaseGPU, sizeof(D3D12_GPU_DESCRIPTOR_HANDLE));
			}

			if (descSet->samplerHeapIncrementSize > 0)
			{
				size_t ptrOffset = mapping.descriptorTableRootParameterOffsetSampler * sizeof(D3D12_GPU_DESCRIPTOR_HANDLE);
				memcpy(ptr + ptrOffset, &descSet->samplerHeapDescSetBaseCPU, sizeof(D3D12_GPU_DESCRIPTOR_HANDLE));
			}
		}

		

	}*/

}		
