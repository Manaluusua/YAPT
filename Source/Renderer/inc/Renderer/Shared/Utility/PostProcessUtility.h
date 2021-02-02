#pragma once

#include <renderer/Shared/Utility/PipelineLayoutHelper.h>
#include <Renderer/Shared/Utility/GpuBufferHelper.h>
#include <Renderer/Shared/Utility/CommonGpuBufferStructs.h>
#include <Renderer/Shared/Utility/PipelineStateDescriptionUtility.h>

namespace YAPT
{
	class CRenderer;
	class RenderNode;
	class ComputeNode;
	struct StaticSamplerEntry
	{
		const char* name;
		SamplerHandle sampler;
	};

	class PostProcessGraphicsPassUtility
	{
	public:
		PostProcessGraphicsPassUtility();
		~PostProcessGraphicsPassUtility();
		void init(CRenderer* r, RenderPassHandle renderPass, const ShaderLoader::ShaderPipelineInfo* pipelineInfo, const StaticSamplerEntry* staticSamplers, size_t numberOfStaticSamplers);
		void createPso();
		void deinit();

		void updateDescriptorSet(size_t index, const DescriptorSetUpdate* updates, size_t updateCount);

		void drawFullscreenPass(CommandBufferHandle commandBuffer);

		PipelineLayoutHelper& getPipelineLayoutHelper()  { return m_layout; }
		GraphicsPipelineStateHandle getPso() { return m_pso; }
		GraphicsPipelineStateDescHelper& getPipelineStateHelper() { return m_psoDesc; }

	private:
		CRenderer* m_renderer;
		PipelineLayoutHelper m_layout;
		GraphicsPipelineStateDescHelper m_psoDesc;
		GraphicsPipelineStateHandle m_pso;
		std::vector<DescriptorSetBinding> m_descSetHandles;
	};

	class PostProcessComputePassUtility
	{
	public:
		PostProcessComputePassUtility();
		~PostProcessComputePassUtility();
		void init(CRenderer* r, const ShaderLoader::ShaderPipelineInfo* pipelineInfo, const StaticSamplerEntry* staticSamplers, size_t numberOfStaticSamplers);


		void createPipelineState();
		void deinit();

		void updateDescriptorSet(size_t index, const DescriptorSetUpdate* updates, size_t updateCount);

		void dispatch(CommandBufferHandle commandBuffer, uint32_t x, uint32_t y, uint32_t z);

		PipelineLayoutHelper& getPipelineLayoutHelper() { return m_layout; }
		ComputePipelineStateHandle getPso() { return m_pso; }

	private:
		CRenderer* m_renderer;
		PipelineLayoutHelper m_layout;
		ComputePipelineStateHandle m_pso;
		ComputePipelineStateDesc m_stateDesc;
		std::vector<DescriptorSetBinding> m_descSetHandles;
	};
}