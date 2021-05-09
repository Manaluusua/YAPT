#include <Renderer/Vk/ShaderPipelineReflectionVk.h>
#include <Renderer/Vk/ShaderModuleVk.h>
#include <common/CommonUtilities.h>
#include <Renderer/RendererCommonTypesUtility.h>
#include <assert.h>
#include <algorithm>

namespace YAPT
{
	ShaderPipelineReflectionVk::ShaderPipelineReflectionVk(ShaderModuleHandle* shaderModules, size_t shaderModuleCount)
	{
		init(shaderModules, shaderModuleCount);
	}
	
	ShaderPipelineReflectionVk::~ShaderPipelineReflectionVk()
	{

	}

	void ShaderPipelineReflectionVk::init(ShaderModuleHandle* shaderModules, size_t shaderModuleCount)
	{

		size_t highestDescSetIndex = 0;

		for (size_t moduleIndex = 0; moduleIndex < shaderModuleCount; ++moduleIndex)
		{
			ShaderModuleVk* mod = shaderModules[moduleIndex];
			if (mod->m_sortedBindings.size() > 0)
			{
				highestDescSetIndex = max(mod->m_sortedBindings.size() - 1, highestDescSetIndex);
			}
		}

		m_reflectionDataPerDescSet.resize(highestDescSetIndex + 1);

		std::unordered_map<uint64_t, ShaderPipelineReflection::ResourceBinding> tempBindings;
		std::unordered_map<uint64_t, std::vector<std::pair<const char*, ShaderModuleType>>> tempNameMappings;

		for (size_t moduleIndex = 0; moduleIndex < shaderModuleCount; ++moduleIndex)
		{
			ShaderModuleVk* mod = shaderModules[moduleIndex];

			for (size_t descSetIndex = 0; descSetIndex < mod->m_sortedBindings.size(); ++descSetIndex)
			{
				const std::vector<ShaderModuleVk::ResourceBinding>& bindingsVk = mod->m_sortedBindings[descSetIndex];
				for (size_t bindingInd = 0; bindingInd < bindingsVk.size(); ++bindingInd)
				{
					const ShaderModuleVk::ResourceBinding& bindingVk = bindingsVk[bindingInd];
					uint64_t ind = (uint64_t)descSetIndex << 32 | bindingVk.bindingIndex;
					ShaderPipelineReflection::ResourceBinding& binding = tempBindings[ind];
					binding.descriptorCount = bindingVk.descriptorCount;
					binding.bindingIndex = bindingVk.bindingIndex;
					binding.accessFlags = bindingVk.accessFlags;
					binding.type = bindingVk.type;
					binding.shaderStages |= shaderModuleTypeToShaderStagesFlag(mod->m_moduleType);

					tempNameMappings[ind].push_back(std::make_pair(bindingVk.name.c_str(), mod->m_moduleType));

				}
			}

			

			if (mod->m_moduleType == ShaderModuleType::VERTEX_MODULE)
			{
				std::vector<ShaderModuleVk::InputAttributes> &inputData = mod->m_inputAttributes;

				for (size_t i = 0; i < inputData.size(); ++i)
				{
					assert(i == inputData[i].location);
					m_inputAttributes.push_back(inputData[i].semantic);
				}
			}
		}


		//
		for (auto iter = tempBindings.begin(); iter != tempBindings.end(); ++iter)
		{
			size_t descsetInd = iter->first >> 32;
			size_t bindingInd = iter->first & 0xFFFF;
			const ShaderPipelineReflection::ResourceBinding& b = iter->second;

			m_reflectionDataPerDescSet[descsetInd].bindingDefinitions.push_back(b);
		}

		for (size_t descSetIndex = 0; descSetIndex < m_reflectionDataPerDescSet.size(); ++descSetIndex)
		{
			std::sort(m_reflectionDataPerDescSet[descSetIndex].bindingDefinitions.data(),
				m_reflectionDataPerDescSet[descSetIndex].bindingDefinitions.data() + m_reflectionDataPerDescSet[descSetIndex].bindingDefinitions.size(),
				[](const ShaderPipelineReflection::ResourceBinding& a, const ShaderPipelineReflection::ResourceBinding& b)
				{
					return a.bindingIndex < b.bindingIndex;
				}
			);
		}


		//setup names 
		for (size_t descSetIndex = 0; descSetIndex < m_reflectionDataPerDescSet.size(); ++descSetIndex)
		{
			size_t bindingCount = m_reflectionDataPerDescSet[descSetIndex].bindingDefinitions.size();
			for (size_t i = 0; i < bindingCount; ++i)
			{
				uint32_t bindIndex = m_reflectionDataPerDescSet[descSetIndex].bindingDefinitions[i].bindingIndex;
				uint64_t ind = (uint64_t)descSetIndex << 32 | bindIndex;

				auto iter = tempNameMappings.find(ind);

				if (iter != tempNameMappings.end())
				{
					for (size_t k = 0; k < iter->second.size(); ++k)
					{
						m_nameMappings[(size_t)iter->second[k].second][iter->second[k].first] = { (uint32_t)descSetIndex, bindIndex};
					}
					
				}
			}
		}
		

	}
}