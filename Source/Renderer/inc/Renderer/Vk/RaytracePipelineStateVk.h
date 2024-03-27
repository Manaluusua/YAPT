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

		uint8_t* getMissShaderRecordHandle(size_t missShaderRecordIndex)
		{
			return getShaderRecordHandle(m_missGroupIndexOffset + missShaderRecordIndex);
		}
		uint8_t* getRayGenShaderRecordHandle(size_t rayGenShaderRecordIndex)
		{
			return getShaderRecordHandle(m_rayGenIndexOffset + rayGenShaderRecordIndex);
		}
		uint8_t* getHitShaderRecordHandle(size_t hitShaderRecordIndex)
		{
			return getShaderRecordHandle(m_hitGroupIndexOffset + hitShaderRecordIndex);
		}


	private:
		uint8_t* getShaderRecordHandle(size_t handleIndex)
		{
			return m_shaderGroupHandles.data() + handleIndex * m_handleSize;
		}

		VkPipeline m_pipeline;
		size_t m_hitGroupConstantsSizeInBytes;
		size_t m_missConstantsSizeInBytes;
		size_t m_rayGenConstantsSizeInBytes;

		size_t m_hitGroupIndexOffset;
		size_t m_missGroupIndexOffset;
		size_t m_rayGenIndexOffset;

		uint32_t m_handleSize;
		std::vector<uint8_t> m_shaderGroupHandles;
	};
}