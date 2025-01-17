#pragma once

#include <Renderer/Vk/CommonVk.h>
#include <Renderer/Shared/GfxTypes.h>
#include <Renderer/Vk/SimpleBufferAllocationUtility.h>

namespace YAPT
{
	class RendererVk;
	class RaytracePipelineStateVk;
	struct BufferHandleVk;
	class ShaderTableVk
	{
	public:

		static ShaderTableVk* createShaderTable(RendererVk* renderer, RaytracePipelineStateHandle pso, size_t numberOfRayGenShaders, size_t numberOfMissShaders, size_t numberOfHitGroups);

		void writeShaderTableEntries(const ShaderTableEntry* rayGenBindings, size_t numberOfRayGenBindings,
			const ShaderTableEntry* missBindings, size_t numberOfMissBindings,
			const ShaderTableEntry* hitGroupBindings, size_t numberOfHitGroupBindings);

		inline VkStridedDeviceAddressRegionKHR getRayGenShaderTableRange() const { return m_rayGenRegion; }
		inline VkStridedDeviceAddressRegionKHR getRayHitShaderTableRange() const { return m_hitRegion; }
		inline VkStridedDeviceAddressRegionKHR getRayMissShaderTableRange() const { return m_missRegion; }

		~ShaderTableVk();

	private:
		ShaderTableVk(RendererVk* renderer, RaytracePipelineStateHandle pso, size_t numberOfRayGenShaders, size_t numberOfMissShaders, size_t numberOfHitGroups);

		void createBuffer(VkDeviceSize bufferSize);
		void releaseBuffer();

		void writeShaderTableEntry(size_t offsetToBufferStart, size_t alignedEntrySize, uint8_t* record, const ShaderTableEntry& entry);
		void uploadShadowBufferData();
		void clearShadowBufferRange();

		size_t getRayGenSectionBufferOffset() const { return 0; }
		size_t getMissSectionBufferOffset() const { return m_rayGenRegion.size; }
		size_t getHitSectionBufferOffset() const { return getMissSectionBufferOffset() + m_missRegion.size; }
		
		RendererVk& m_renderer;
		RaytracePipelineStateVk& m_pso;
		
		VkStridedDeviceAddressRegionKHR m_rayGenRegion;
		VkStridedDeviceAddressRegionKHR m_missRegion;
		VkStridedDeviceAddressRegionKHR m_hitRegion;
		uint32_t m_shaderGroupHandleSize;

		SimpleBufferAllocationUtility m_buffer;

		std::vector<uint8_t> m_shadowBuffer;
		size_t m_shadowBufferUploadRangeMin;
		size_t m_shadowBufferUploadRangeMax;
	};

}
