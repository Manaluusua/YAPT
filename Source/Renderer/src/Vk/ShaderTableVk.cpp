#include <Renderer/Vk/ShaderTableVk.h>
#include <Common/CommonUtilities.h>
namespace YAPT
{
	ShaderTableVk* ShaderTableVk::createShaderTable(RendererVk* renderer, RaytracePipelineStateHandle pso, size_t numberOfRayGenShaders, size_t numberOfMissShaders, size_t numberOfHitGroups)
	{
		assert(!"NOT IMPLEMENTED!");
		return new ShaderTableVk(renderer, pso, numberOfRayGenShaders, numberOfMissShaders, numberOfHitGroups);
	}


	ShaderTableVk::~ShaderTableVk()
	{
		assert(!"NOT IMPLEMENTED!");
	}

	ShaderTableVk::ShaderTableVk(RendererVk* renderer, RaytracePipelineStateHandle pso, size_t numberOfRayGenShaders, size_t numberOfMissShaders, size_t numberOfHitGroups)
	{
		const auto& props = renderer->getPhysicalDeviceInfo().raytracePipelineProperties;

		m_shaderRecordAlignedSize = align(props.shaderGroupHandleSize, props.shaderGroupHandleAlignment);
	}
}