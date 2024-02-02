#pragma once


#include <Renderer/Vk/CommonVk.h>
#include <Renderer/Shared/GfxTypes.h>
#include <Renderer/Vk/RendererVk.h>
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

		uint32_t m_shaderRecordAlignedSize;

	};

}
