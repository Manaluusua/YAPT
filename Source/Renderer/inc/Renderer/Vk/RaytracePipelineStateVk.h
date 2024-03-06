#pragma once

#include <Renderer/Vk/CommonVk.h>
#include <Renderer/Shared/GfxTypes.h>
namespace YAPT
{

	class RaytracePipelineStateVk
	{
	public:
		static RaytracePipelineStateVk* create(RendererVk* renderer, const RaytracePipelineStateDesc& desc);
		static void destroy(RendererVk* renderer, RaytracePipelineStateVk* pipeline);

		VkPipeline getPipeline() const { return m_pipeline; }
		size_t getHitGroupConstantsSizeInBytes() const { return m_hitGroupConstantsSizeInBytes; }
		size_t getMissConstantsSizeInBytes() const { return m_missConstantsSizeInBytes; }
		size_t getRayGenConstantsSizeInBytes() const { return m_rayGenConstantsSizeInBytes; }

		uint8_t* getHandle(size_t handleIndex)
		{
			return m_handles.data() + handleIndex * m_handleSize;
		}


	private:
		VkPipeline m_pipeline;
		size_t m_hitGroupConstantsSizeInBytes;
		size_t m_missConstantsSizeInBytes;
		size_t m_rayGenConstantsSizeInBytes;

		uint32_t m_handleSize;
		std::vector<uint8_t> m_handles;
	};
}