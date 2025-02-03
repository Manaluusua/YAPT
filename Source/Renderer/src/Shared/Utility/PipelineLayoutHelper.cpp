#include <Renderer/Shared/Utility/PipelineLayoutHelper.h>
#include <Gfx/GfxApi.h>
#include <Common/CommonUtilities.h>
#include <algorithm>
#include <Common/Logger.h>
#include <assert.h>
namespace YAPT
{
	uint64_t packToLayoutDefKey(uint32_t descSet, uint32_t bindingPoint)
	{
		uint64_t key = uint64_t(descSet) << 32 | bindingPoint;
		return key;
	}

	PipelineLayoutHelper::PipelineLayoutHelper()
		:m_gfx(YAPT_NULL_HANDLE),
		m_pipelineLayout(YAPT_NULL_HANDLE)

	{

	}
	PipelineLayoutHelper::~PipelineLayoutHelper()
	{
		if (m_pipelineLayout != YAPT_NULL_HANDLE)
		{
			Gfx::destroyPipelineLayout(m_gfx, m_pipelineLayout);
		}

		for (size_t i = 0; i < m_descriptorSetLayouts.size(); ++i)
		{
			Gfx::destroyDescriptorSetLayout(m_gfx, m_descriptorSetLayouts[i]);
		}
	}

	void PipelineLayoutHelper::initFromShaderReflection(GfxApiHandle gfx, ShaderPipelineReflection* refl)
	{
		m_gfx = gfx;
		m_descriptorSetLayoutDefinitions.resize(refl->getHighestDescSetIndex() + 1);
		for (size_t descSetIndex = 0; descSetIndex < m_descriptorSetLayoutDefinitions.size(); ++descSetIndex)
		{
			DescriptorSetLayoutDef& layoutDef = m_descriptorSetLayoutDefinitions[descSetIndex];
			const ShaderPipelineReflection::ResourceBinding* shdResBindings = refl->getBindingDefinitionsForDescSet(descSetIndex);
			size_t numBindings = refl->getNumberOfBindingDefinitionsForDescSet(descSetIndex);
			layoutDef.bindings.resize(numBindings);
			layoutDef.extraData.resize(numBindings);
			for (size_t bindingDefinitionIndex = 0; bindingDefinitionIndex < numBindings; ++bindingDefinitionIndex)
			{
				const ShaderPipelineReflection::ResourceBinding& shdResBinding = shdResBindings[bindingDefinitionIndex];
				DescriptorSetLayoutBinding& binding = layoutDef.bindings[bindingDefinitionIndex];

				binding.accessFlags = shdResBinding.accessFlags;
				binding.bindingIndex = shdResBinding.bindingIndex;
				binding.descriptorCount = shdResBinding.descriptorCount;
				binding.shaderStages = shdResBinding.shaderStages;
				binding.type = shdResBinding.type;
				binding.staticSamplers = nullptr;

				m_bindingPointToDescriptorSetLayoutDefIndex[packToLayoutDefKey((uint32_t)descSetIndex, binding.bindingIndex)] = bindingDefinitionIndex;
			}

		}

		m_descriptorSetLayouts.resize(m_descriptorSetLayoutDefinitions.size(), YAPT_NULL_HANDLE);
	}

	void PipelineLayoutHelper::setExplicitDescriptorSetLayout(size_t descSetIndex, const DescriptorSetLayoutBinding* bindings, size_t bindingCount, DescriptorSetLayoutHandle layoutHandle)
	{
		m_descriptorSetLayoutDefinitions[descSetIndex].bindings.assign(bindings, bindings + bindingCount);
		m_descriptorSetLayouts[descSetIndex] = layoutHandle;
	}

	void PipelineLayoutHelper::initRaytraceLayoutFromShaderReflections(GfxApiHandle gfx, ShaderPipelineReflection** reflections, size_t numberOfReflectionData)
	{
		m_gfx = gfx;
		 
		size_t highestDescSetIndex = 0;

		for (size_t reflIndex = 0; reflIndex < numberOfReflectionData; ++reflIndex)
		{
			ShaderPipelineReflection* refl = reflections[reflIndex];
			highestDescSetIndex = max(highestDescSetIndex, refl->getHighestDescSetIndex());
		}

		m_descriptorSetLayoutDefinitions.resize(highestDescSetIndex + 1);
		 
		std::unordered_map<size_t, DescriptorSetLayoutBinding> bindings;

		for (size_t descSetIndex = 0; descSetIndex < m_descriptorSetLayoutDefinitions.size(); ++descSetIndex)
		{
			bindings.clear();
			for (size_t reflIndex = 0; reflIndex < numberOfReflectionData; ++reflIndex)
			{
				ShaderPipelineReflection* refl = reflections[reflIndex];
				const ShaderPipelineReflection::ResourceBinding* shdResBindings = refl->getBindingDefinitionsForDescSet(descSetIndex);
				if (shdResBindings == nullptr) continue;
				size_t numBindings = refl->getNumberOfBindingDefinitionsForDescSet(descSetIndex);

				for (size_t bindingDefinitionIndex = 0; bindingDefinitionIndex < numBindings; ++bindingDefinitionIndex)
				{
					const ShaderPipelineReflection::ResourceBinding& shdResBinding = shdResBindings[bindingDefinitionIndex];

					DescriptorSetLayoutBinding* dstBinding;

					auto iter = bindings.find(shdResBinding.bindingIndex);

					if (iter != bindings.end())
					{
						dstBinding = &iter->second;

						//simple sanity check
						assert(dstBinding->accessFlags == shdResBinding.accessFlags);
						assert(dstBinding->bindingIndex == shdResBinding.bindingIndex);
						assert(dstBinding->descriptorCount == shdResBinding.descriptorCount);
						assert(dstBinding->type == shdResBinding.type);
						dstBinding->shaderStages |= shdResBinding.shaderStages;
					}
					else
					{
						dstBinding = &bindings[shdResBinding.bindingIndex];
						dstBinding->accessFlags = shdResBinding.accessFlags;
						dstBinding->bindingIndex = shdResBinding.bindingIndex;
						dstBinding->descriptorCount = shdResBinding.descriptorCount;
						dstBinding->type = shdResBinding.type;
						dstBinding->staticSamplers = nullptr;
						dstBinding->shaderStages = shdResBinding.shaderStages;
					}
				}
			}

			DescriptorSetLayoutDef& layoutDef = m_descriptorSetLayoutDefinitions[descSetIndex];
			layoutDef.bindings.reserve(bindings.size());
			layoutDef.extraData.resize(bindings.size());

			for (auto iter = bindings.begin(); iter != bindings.end(); ++iter)
			{
				layoutDef.bindings.push_back(iter->second);
			}

			std::sort(layoutDef.bindings.begin(), layoutDef.bindings.end(),
				[](const DescriptorSetLayoutBinding& a, const DescriptorSetLayoutBinding& b)
				{
					return a.bindingIndex < b.bindingIndex;
				});
		}
		m_descriptorSetLayouts.resize(m_descriptorSetLayoutDefinitions.size(), YAPT_NULL_HANDLE);

		for (size_t descSetIndex = 0; descSetIndex < m_descriptorSetLayoutDefinitions.size(); ++descSetIndex)
		{
			const DescriptorSetLayoutDef& def = m_descriptorSetLayoutDefinitions[descSetIndex];
			for (size_t bindingDefIndex = 0; bindingDefIndex < def.bindings.size(); ++bindingDefIndex)
			{
				m_bindingPointToDescriptorSetLayoutDefIndex[packToLayoutDefKey((uint32_t)descSetIndex, def.bindings[bindingDefIndex].bindingIndex)] = bindingDefIndex;
			}

		}
	}
	    
	void PipelineLayoutHelper::setStaticSamplers(const ShaderPipelineReflection::NameMapping& mapping, SamplerHandle* samplers)
	{
		setStaticSamplers(mapping.descSetIndex, mapping.bindingPoint, samplers);
	}

	void PipelineLayoutHelper::setStaticSamplers(uint32_t descSetInd, uint32_t bindingPoint, SamplerHandle* samplers)
	{
		assert(descSetInd < m_descriptorSetLayoutDefinitions.size());
		assert(bindingPoint <= m_descriptorSetLayoutDefinitions[descSetInd].bindings.back().bindingIndex);

		auto iter = m_bindingPointToDescriptorSetLayoutDefIndex.find(packToLayoutDefKey(descSetInd, bindingPoint));

		if (iter == m_bindingPointToDescriptorSetLayoutDefIndex.end())
		{
			YAPT_LOG_FATAL_ERROR("Can't set static sampler to descset %d, binding point %d, can't find definition for it", descSetInd, bindingPoint);
			return;
		}

		size_t defIndex = iter->second;

		DescriptorSetLayoutBinding& binding = m_descriptorSetLayoutDefinitions[descSetInd].bindings[defIndex];
		ExtraData& extra = m_descriptorSetLayoutDefinitions[descSetInd].extraData[defIndex];
		
		extra.samplerHandles.assign(samplers, samplers + binding.descriptorCount);

		m_descriptorSetLayoutDefinitions[descSetInd].bindings[defIndex].staticSamplers = extra.samplerHandles.data();
		
	}

	void PipelineLayoutHelper::setDynamic(size_t descSetIndex, size_t bindingPointIndex)
	{
		if (descSetIndex >= m_descriptorSetLayoutDefinitions.size()) return;

		for (size_t i = 0; i < m_descriptorSetLayoutDefinitions[descSetIndex].bindings.size(); ++i)
		{
			DescriptorSetLayoutBinding& binding = m_descriptorSetLayoutDefinitions[descSetIndex].bindings[i];
			if (binding.bindingIndex != bindingPointIndex) continue;

			if (binding.type == DescriptorType::UNIFORM_BUFFER)
			{
				binding.type = DescriptorType::UNIFORM_BUFFER_DYNAMIC;
			}
			else if (binding.type == DescriptorType::STORAGE_BUFFER)
			{
				binding.type = DescriptorType::STORAGE_BUFFER_DYNAMIC;
			}
		}

	}


	void PipelineLayoutHelper::compile()
	{
		assert(m_gfx != YAPT_NULL_HANDLE);
		m_descriptorSetUtilities.resize(m_descriptorSetLayoutDefinitions.size());
		for (size_t i = 0; i < m_descriptorSetLayouts.size(); ++i)
		{
			if (m_descriptorSetLayouts[i] == YAPT_NULL_HANDLE)
			{
				m_descriptorSetLayouts[i] = Gfx::createDescriptorSetLayout(m_gfx, m_descriptorSetLayoutDefinitions[i].bindings.data(), m_descriptorSetLayoutDefinitions[i].bindings.size(), DESCRIPTORSETLAYOUTFLAG_NONE);
			}
			
			m_descriptorSetUtilities[i].init(m_gfx, m_descriptorSetLayouts[i]);
		}
		m_pipelineLayout = Gfx::createPipelineLayout(m_gfx, m_descriptorSetLayouts.data(), m_descriptorSetLayouts.size());
		 
	}

	DescriptorSetHandle PipelineLayoutHelper::allocateDescriptorSet(size_t index)
	{
		assert(index < m_descriptorSetUtilities.size());
		return m_descriptorSetUtilities[index].getNewDescriptorSet();
	}
	void PipelineLayoutHelper::deallocateDescriptorSet(size_t index, DescriptorSetHandle handle)
	{
		m_descriptorSetUtilities[index].freeDescriptorSet(handle);
	}

}