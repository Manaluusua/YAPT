#pragma once

#include <Renderer/Dx12/Dx12CommonIncludes.h>
#include <Renderer/Dx12/RaytracePipelineStateDx12.h>

namespace YAPT
{
	class ResourceManagerDx12;
	class ResourceAllocationPoolDx12;
	struct BufferHandleDx12;
	struct ShaderTableBinding;

	class ShaderTableDx12
	{
	public:
		ShaderTableDx12(ResourceManagerDx12& resMngr, RaytracePipelineStateDx12* pso, size_t numberOfRayGenShaders, size_t numberOfMissShaders, size_t numberOfHitGroups, bool useSeparateBufferForTypes = false);
		~ShaderTableDx12();
		
		void commit(const ShaderTableEntry* rayGenBindings, size_t numberOfRayGenBindings,
			const ShaderTableEntry* missBindings, size_t numberOfMissBindings,
			const ShaderTableEntry* hitGroupBindings, size_t numberOfHitGroupBindings);

		size_t getRayGenShaderSectionAlignedSizeInBytes() const;
		size_t getMissShaderSectionAlignedSizeInBytes() const;
		size_t getHitGroupSectionAlignedSizeInBytes() const;

		size_t getRayGenShaderSectionSizeInBytes() const;
		size_t getMissShaderSectionSizeInBytes() const;
		size_t getHitGroupSectionSizeInBytes() const;

		size_t getRayGenShaderEntryMaxAlignedSizeInBytes() const { return m_rayGenEntrySizeInBytes; }
		size_t getMissShaderEntryMaxAlignedSizeInBytes() const { return m_missEntrySizeInBytes; }
		size_t getHitGroupEntryMaxAlignedSizeInBytes() const { return m_hitGroupEntrySizeInBytes; }


		D3D12_GPU_VIRTUAL_ADDRESS getRayGenShaderTableGpuBaseAddress() const;
		D3D12_GPU_VIRTUAL_ADDRESS getMissShaderTableGpuBaseAddress() const;
		D3D12_GPU_VIRTUAL_ADDRESS getHitGroupShaderTableGpuBaseAddress() const;

	private:

		enum SeparatedBufferIndex
		{
			RayGenIndex,
			MissIndex,
			HitGroupIndex
		};

		void writeShaderEntry(void* dst, const RaytracePipelineStateDx12::ShaderRecordInfo& record, const ShaderTableEntry& entry);
		//void writeShaderEntry(void* dst, const RaytracePipelineStateDx12::ShaderRecordInfo& shaderRecordInfo, DescriptorSetBinding* bindings, size_t bindingsCount);
		void flushShadowBufferToGPU(bool rayGenDirty, bool missDirty, bool hitGroupDirty);

		size_t getOffsetToRayGenShaderSectionInBytes() const;
		size_t getOffsetToMissShaderSectionInBytes() const;
		size_t getOffsetToHitGroupSectionInBytes() const;

		size_t getActualBufferCount() const { return m_separateBuffersPerType ? 3 : 1; }

		ResourceManagerDx12& m_resMngr;

		RaytracePipelineStateDx12* m_pso;

		BufferHandleDx12* m_bufferHandles[3];
		ResourceAllocationPoolDx12* m_resourceAllocationPool;

		std::vector<uint8_t> m_shadowBuffer;

		size_t m_numberOfRayGenShaders;
		size_t m_numberOfMissShaders;
		size_t m_numberOfHitGroups;

		size_t m_rayGenEntrySizeInBytes;
		size_t m_missEntrySizeInBytes;
		size_t m_hitGroupEntrySizeInBytes;

		bool m_separateBuffersPerType;
	};
}
