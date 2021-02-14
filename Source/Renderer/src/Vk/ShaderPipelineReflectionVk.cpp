#include <Renderer/Vk/ShaderPipelineReflectionVk.h>

namespace YAPT
{
	ShaderPipelineReflectionVk::ShaderPipelineReflectionVk(ShaderModuleHandle* shaderModules, size_t shaderModuleCount)
	{

	}
	
	ShaderPipelineReflectionVk::~ShaderPipelineReflectionVk()
	{

	}

	void ShaderPipelineReflectionVk::init(ShaderModuleHandle* shaderModules, size_t shaderModuleCount)
	{
		/*size_t highestDescSetIndex = 0;

		for (size_t moduleIndex = 0; moduleIndex < shaderModuleCount; ++moduleIndex)
		{
			ShaderModuleDx12* mod = shaderModules[moduleIndex];
			if (mod->reflection.sortedBindings.size() > 0)
			{
				highestDescSetIndex = max(mod->reflection.sortedBindings.back().spaceIndex, (uint32_t)highestDescSetIndex);
			}
		}

		m_reflectionDataPerDescSet.resize(highestDescSetIndex + 1);

		//using hashmap here to collect only unique entries (and assuming bindings have gaps)
		std::unordered_map<uint64_t, ShaderPipelineReflection::ResourceBinding> tempBindings;
		std::unordered_map<uint64_t, std::vector<std::pair<const char*, ShaderModuleType>>> tempNameMappings;

		for (size_t moduleIndex = 0; moduleIndex < shaderModuleCount; ++moduleIndex)
		{
			ShaderModuleDx12* mod = shaderModules[moduleIndex];
			for(size_t bindingInd = 0; bindingInd < mod->reflection.sortedBindings.size(); ++bindingInd)
			{
				const ShaderReflectionDataBindingDx12& bindingDx12 = mod->reflection.sortedBindings[bindingInd];

				//handle shader table extra data as a special case, it cannot be bound directly and can't be queried
				if (bindingDx12.name == SHADERTABLE_EXTRADATA_NAME)
				{
					assert(bindingDx12.spaceIndex == 0);
					assert(bindingDx12.bindPoint == 0);
					assert(bindingDx12.type == DescriptorType::UNIFORM_BUFFER);
					assert(bindingDx12.bindCount == 1);
           
					m_hasShaderTableBuffer = true;
				}
				else
				{
					uint64_t ind = (uint64_t)bindingDx12.spaceIndex << 32 | bindingDx12.bindPoint;
					ShaderPipelineReflection::ResourceBinding& binding = tempBindings[ind];
					binding.descriptorCount = bindingDx12.bindCount;
					binding.bindingIndex = bindingDx12.bindPoint;
					binding.accessFlags = bindingDx12.accessFlags;
					binding.type = bindingDx12.type;
					binding.accessFlags |= shaderModuleTypeToShaderStagesFlag(mod->type);

					tempNameMappings[ind].push_back(std::make_pair(bindingDx12.name.c_str(), mod->type));
				}

				
			}

			if (mod->type == ShaderModuleType::VERTEX_MODULE)
			{
				std::vector<ShaderReflectionDataVertexInputEntryDx12> &inputData = mod->reflection.sortedInputData;

				for (size_t i = 0; i < inputData.size(); ++i)
				{
					assert(i == inputData[i].index);
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
		*/

	}
}