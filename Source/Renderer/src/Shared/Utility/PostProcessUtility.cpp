#include <Renderer/Shared/Utility/PostProcessUtility.h>
#include <Renderer/Shared/CRenderer.h>
#include <Gfx/RenderGraph/RenderNode.h>
#include <Renderer/Shared/Utility/CoreRenderResourcesUtility.h>
namespace YAPT
{
	////////////////////////////////////////////////////////////////////////////// PostProcessGraphicsPassUtility Impl //////////////////////////////////////////////////////////////////////////////
	PostProcessGraphicsPassUtility::PostProcessGraphicsPassUtility()
		:m_renderer(nullptr),
		m_pso(YAPT_NULL_HANDLE)

	{

	}
	PostProcessGraphicsPassUtility::~PostProcessGraphicsPassUtility()
	{
		deinit();
	}
	void PostProcessGraphicsPassUtility::init(CRenderer* r, RenderPassHandle renderPass, const ShaderLoader::ShaderPipelineInfo* pipelineInfo, const StaticSamplerEntry* staticSamplers, size_t numberOfStaticSamplers, const ExplicitDescriptorSetDefinition* explicitDescSetDefs , size_t numberOfExplicitDescSetDefs)
	{
		m_renderer = r;
		m_layout.initFromShaderReflection(m_renderer->getGfxHandle(), pipelineInfo->reflection);

		for (size_t i = 0; i < numberOfStaticSamplers; ++i)
		{
			const StaticSamplerEntry& s = staticSamplers[i];
			ShaderPipelineReflection::NameMapping nameMapping;
			bool found = pipelineInfo->reflection->getNameMapping(ShaderModuleType::FRAGMENT_MODULE, s.name, nameMapping);
			if (!found)
			{
				YAPT_LOG_ERROR("Unable to bind static sampler, not sampler named %s found from fragment module", s.name);
				continue;
			}
			SamplerHandle sampler = s.sampler;
			m_layout.setStaticSamplers(nameMapping, &sampler);
		}

		for (size_t i = 0; i < numberOfExplicitDescSetDefs; ++i)
		{
			m_layout.setExplicitDescriptorSetLayout(explicitDescSetDefs[i].descSetIndex, explicitDescSetDefs[i].bindings, explicitDescSetDefs[i].bindingCount, explicitDescSetDefs[i].layoutHandle);
		}

		m_layout.compile();

		m_psoDesc.setupShaderStages(pipelineInfo)
			.setVertexBufferDefinitions(m_renderer->getCoreResources()->getDefaultVertexBufferDefinition(DefaultBufferType::FULLSCREEN_PRIMITIVE_POS_UV), 1);
		m_psoDesc.pipelineDesc.renderPass = renderPass;
		m_psoDesc.pipelineDesc.pipelineLayout = m_layout.getPipelineLayoutHandle();

		m_descSetHandles.resize(m_layout.getDescriptorSetCount());
		for (size_t i = 0; i < m_layout.getDescriptorSetCount(); ++i)
		{
			m_descSetHandles[i] = YAPT_NULL_HANDLE;
		}
	}

	void PostProcessGraphicsPassUtility::createPso()
	{

		if (m_pso != YAPT_NULL_HANDLE)
		{
			Gfx::destroyGraphicsPipelineState(m_renderer->getGfxHandle(), m_pso);
		}
		m_pso = Gfx::createGraphicsPipelineState(m_renderer->getGfxHandle(), m_psoDesc.pipelineDesc);
	}

	void PostProcessGraphicsPassUtility::deinit()
	{
		if (m_pso != YAPT_NULL_HANDLE)
		{
			Gfx::destroyGraphicsPipelineState(m_renderer->getGfxHandle(), m_pso);
			m_pso = YAPT_NULL_HANDLE;
		}
	}

	void PostProcessGraphicsPassUtility::reserveNewDescriptorSet(size_t index)
	{
		freeDescSet(index);
		DescriptorSetHandle newDescSet = m_layout.getDescriptorSetUtility(index).getNewDescriptorSet();
		m_descSetHandles[index] = newDescSet;
	}

	void PostProcessGraphicsPassUtility::reserveAndUpdateDescriptorSet(size_t index, const DescriptorSetUpdate* updates, size_t updateCount)
	{
		
		reserveNewDescriptorSet(index);
		Gfx::updateDescriptorSet(m_renderer->getGfxHandle(), m_descSetHandles[index], updates, updateCount);
	}

	void PostProcessGraphicsPassUtility::setExternallyOwnedDescriptorSet(size_t index, DescriptorSetHandle handle)
	{
		m_descSetHandles[index] = handle;
		m_externallyOwnedDescSets |= 1ull << index;
	}

	void PostProcessGraphicsPassUtility::drawFullscreenPass(CommandBufferHandle commandBuffer)
	{
		GfxApiHandle gfx = m_renderer->getGfxHandle();
		Gfx::setGraphicsPipelineState(gfx, commandBuffer, m_pso);

		
		Gfx::bindDescriptorSets(gfx, commandBuffer, BindingPoint::BINDING_POINT_GRAPHICS, m_layout.getPipelineLayoutHandle(), m_descSetHandles.data(), 0, m_descSetHandles.size(), nullptr, 0);

		BufferViewHandle vertexBuffers[] = { m_renderer->getCoreResources()->getDefaultBufferView(DefaultBufferType::FULLSCREEN_PRIMITIVE_POS_UV) };

		Gfx::setVertexBuffers(gfx, commandBuffer, vertexBuffers, 1, 0);
		Gfx::setIndexBuffer(gfx, commandBuffer, m_renderer->getCoreResources()->getDefaultBufferView(DefaultBufferType::FULLSCREEN_PRIMITIVE_INDICES));

		Gfx::drawIndexed(gfx, commandBuffer, 3, 1, 0, 0, 0);
	}

	void PostProcessGraphicsPassUtility::freeDescSet(size_t index)
	{
		if (m_descSetHandles[index] == YAPT_NULL_HANDLE) return;

		uint64_t descSetMask = (1ull << index);
		if ((m_externallyOwnedDescSets & descSetMask) == 0)
		{
			m_layout.getDescriptorSetUtility(index).freeDescriptorSet(m_descSetHandles[index]);
		}
		else
		{
			m_externallyOwnedDescSets &= ~descSetMask;
		}

	}


	////////////////////////////////////////////////////////////////////////////// PostProcessComputePassUtility Impl //////////////////////////////////////////////////////////////////////////////

	PostProcessComputePassUtility::PostProcessComputePassUtility()
		:m_renderer(nullptr),
		m_pso(YAPT_NULL_HANDLE)
	{

	}
	PostProcessComputePassUtility::~PostProcessComputePassUtility()
	{
		deinit();
	}
	void PostProcessComputePassUtility::init(CRenderer* r, const ShaderLoader::ShaderPipelineInfo* pipelineInfo, const StaticSamplerEntry* staticSamplers, size_t numberOfStaticSamplers, const ExplicitDescriptorSetDefinition* explicitDescSetDefs, size_t numberOfExplicitDescSetDefs)
	{
		m_renderer = r;
		m_layout.initFromShaderReflection(m_renderer->getGfxHandle(), pipelineInfo->reflection);

		for (size_t i = 0; i < numberOfStaticSamplers; ++i)
		{
			const StaticSamplerEntry& s = staticSamplers[i];
			ShaderPipelineReflection::NameMapping nameMapping;
			bool found = pipelineInfo->reflection->getNameMapping(ShaderModuleType::COMPUTE_MODULE, s.name, nameMapping);
			if (!found)
			{
				YAPT_LOG_ERROR("Unable to bind static sampler, not sampler named %s found from fragment module", s.name);
				continue;
			}
			SamplerHandle sampler = s.sampler;
			m_layout.setStaticSamplers(nameMapping, &sampler);
		}

		for (size_t i = 0; i < numberOfExplicitDescSetDefs; ++i)
		{
			m_layout.setExplicitDescriptorSetLayout(explicitDescSetDefs[i].descSetIndex, explicitDescSetDefs[i].bindings, explicitDescSetDefs[i].bindingCount, explicitDescSetDefs[i].layoutHandle);
		}

		m_layout.compile();

		m_descSetHandles.resize(m_layout.getDescriptorSetCount());
		for (size_t i = 0; i < m_layout.getDescriptorSetCount(); ++i)
		{
			m_descSetHandles[i] = YAPT_NULL_HANDLE;
		}
		m_stateDesc.pipelineLayout = m_layout.getPipelineLayoutHandle();
		fillShaderModuleCreateInfo(pipelineInfo->shaderModules[0], m_stateDesc.shaderStage);
	}


	void PostProcessComputePassUtility::createPipelineState()
	{
		if (m_pso != YAPT_NULL_HANDLE)
		{
			Gfx::destroyComputePipelineState(m_renderer->getGfxHandle(), m_pso);
		}

		m_pso = Gfx::createComputePipelineState(m_renderer->getGfxHandle(), m_stateDesc);
	}
	void PostProcessComputePassUtility::deinit()
	{
		if (m_pso != YAPT_NULL_HANDLE)
		{
			Gfx::destroyComputePipelineState(m_renderer->getGfxHandle(), m_pso);
			m_pso = YAPT_NULL_HANDLE;
		}
	}

	void PostProcessComputePassUtility::reserveNewDescriptorSet(size_t index)
	{
		freeDescSet(index);
		DescriptorSetHandle newDescSet = m_layout.getDescriptorSetUtility(index).getNewDescriptorSet();
		m_descSetHandles[index] = newDescSet;
	}

	void PostProcessComputePassUtility::reserveAndUpdateDescriptorSet(size_t index, const DescriptorSetUpdate* updates, size_t updateCount)
	{
		reserveNewDescriptorSet(index);
		Gfx::updateDescriptorSet(m_renderer->getGfxHandle(), m_descSetHandles[index], updates, updateCount);
	}

	void PostProcessComputePassUtility::setExternallyOwnedDescriptorSet(size_t index, DescriptorSetHandle handle)
	{
		m_descSetHandles[index] = handle;
		m_externallyOwnedDescSets |= 1ull << index;
	}

	void PostProcessComputePassUtility::dispatch(CommandBufferHandle commandBuffer, uint32_t x, uint32_t y, uint32_t z)
	{
		GfxApiHandle gfx = m_renderer->getGfxHandle();
		Gfx::setComputePipelineState(gfx, commandBuffer, m_pso);

		Gfx::bindDescriptorSets(gfx, commandBuffer, BindingPoint::BINDING_POINT_COMPUTE, m_layout.getPipelineLayoutHandle(), m_descSetHandles.data(), 0, m_descSetHandles.size(), nullptr, 0);

		Gfx::dispatch(gfx, commandBuffer, x,y,z);
	}

	void PostProcessComputePassUtility::freeDescSet(size_t index)
	{
		if (m_descSetHandles[index] == YAPT_NULL_HANDLE) return;

		uint64_t descSetMask = (1ull << index);
		if ((m_externallyOwnedDescSets & descSetMask) == 0)
		{
			m_layout.getDescriptorSetUtility(index).freeDescriptorSet(m_descSetHandles[index]);
		}
		else
		{
			m_externallyOwnedDescSets &= ~descSetMask;
		}

	}
}