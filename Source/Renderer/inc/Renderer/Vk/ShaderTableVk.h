#pragma once


#include <Renderer/Vk/CommonVk.h>
#include <Renderer/Shared/GfxTypes.h>
namespace YAPT
{
	class RendererVk;
	class ShaderTableVk
	{
	public:

		static ShaderTableVk* createShaderTable(RendererVk* renderer, RaytracePipelineStateHandle pso, size_t numberOfRayGenShaders, size_t numberOfMissShaders, size_t numberOfHitGroups);

		
		~ShaderTableVk();

	private:
		ShaderTableVk(RendererVk* renderer, RaytracePipelineStateHandle pso, size_t numberOfRayGenShaders, size_t numberOfMissShaders, size_t numberOfHitGroups);

		void createBuffer(VkDeviceSize bufferSize);
		void releaseBuffer();

		
		RendererVk& m_renderer;

		
		VkStridedDeviceAddressRegionKHR m_rayGenRegion;
		VkStridedDeviceAddressRegionKHR m_missRegion;
		VkStridedDeviceAddressRegionKHR m_hitRegion;
		

		VkBuffer m_buffer;
		VkDeviceMemory m_memory;
	};

}
