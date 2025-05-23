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

	struct ExplicitDescriptorSetDefinition
	{
		size_t descSetIndex;
		const DescriptorSetLayoutBinding* bindings;
		size_t bindingCount;
		DescriptorSetLayoutHandle layoutHandle;
	};

	class PostProcessGraphicsPassUtility
	{
	public:
		PostProcessGraphicsPassUtility();
		~PostProcessGraphicsPassUtility();
		void init(CRenderer* r, RenderPassHandle renderPass, const ShaderLoader::ShaderPipelineInfo* pipelineInfo, const StaticSamplerEntry* staticSamplers, size_t numberOfStaticSamplers, const ExplicitDescriptorSetDefinition* explicitDescSetDefs = nullptr, size_t numberOfExplicitDescSetDefs = 0 );
		void createPso();
		void deinit();

		void reserveNewDescriptorSet(size_t index);
		void reserveAndUpdateDescriptorSet(size_t index, const DescriptorSetUpdate* updates, size_t updateCount);
		void setExternallyOwnedDescriptorSet(size_t index, DescriptorSetHandle handle);

		void drawFullscreenPass(CommandBufferHandle commandBuffer);

		PipelineLayoutHelper& getPipelineLayoutHelper()  { return m_layout; }
		GraphicsPipelineStateHandle getPso() { return m_pso; }
		GraphicsPipelineStateDescHelper& getPipelineStateHelper() { return m_psoDesc; }

	private:

		void freeDescSet(size_t index);

		CRenderer* m_renderer;
		PipelineLayoutHelper m_layout;
		GraphicsPipelineStateDescHelper m_psoDesc;
		GraphicsPipelineStateHandle m_pso;
		std::vector<DescriptorSetHandle> m_descSetHandles;
		uint64_t m_externallyOwnedDescSets;
	};

	class PostProcessComputePassUtility
	{
	public:
		PostProcessComputePassUtility();
		~PostProcessComputePassUtility();
		void init(CRenderer* r, const ShaderLoader::ShaderPipelineInfo* pipelineInfo, const StaticSamplerEntry* staticSamplers, size_t numberOfStaticSamplers, const ExplicitDescriptorSetDefinition* explicitDescSetDefs = nullptr, size_t numberOfExplicitDescSetDefs = 0);


		void createPipelineState();
		void deinit();

		void reserveNewDescriptorSet(size_t index);
		void reserveAndUpdateDescriptorSet(size_t index, const DescriptorSetUpdate* updates, size_t updateCount);

		void setExternallyOwnedDescriptorSet(size_t index, DescriptorSetHandle handle);

		void dispatch(CommandBufferHandle commandBuffer, uint32_t x, uint32_t y, uint32_t z);
		DescriptorSetHandle getDescriptorSet(size_t index)
		{ 
			if (index >= m_descSetHandles.size())
			{
				return YAPT_NULL_HANDLE;
			}

			return m_descSetHandles[index]; 
		}
		PipelineLayoutHelper& getPipelineLayoutHelper() { return m_layout; }
		ComputePipelineStateHandle getPso() { return m_pso; }

	private:

		void freeDescSet(size_t index);

		CRenderer* m_renderer;
		PipelineLayoutHelper m_layout;
		ComputePipelineStateHandle m_pso;
		ComputePipelineStateDesc m_stateDesc;
		std::vector<DescriptorSetHandle> m_descSetHandles;
		uint64_t m_externallyOwnedDescSets;
	};
}