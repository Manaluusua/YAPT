#include <Gfx/Vk/ShaderModuleVk.h>
#include <Gfx/Utility/DXCUtility.h>
#include <Common/CommonWindowsUtility.h>
#include <Common/CommonUtilities.h>
#include <spirv_reflect.h>
#include <assert.h>
#include <algorithm>
namespace YAPT
{
	const static LPCWSTR extraCompilerParams[] =
	{
		{L"-spirv"},
		{L"-fspv-target-env=vulkan1.2"},
		
		{L"-fspv-extension=SPV_KHR_ray_tracing"},
		{L"-fspv-extension=SPV_EXT_descriptor_indexing"},
		{L"-fspv-extension=SPV_KHR_ray_query"},
		
		/*{L"-fspv-reflect"},
		{L"-fspv-extension=SPV_GOOGLE_hlsl_functionality1"},
		{L"-fspv-extension=SPV_GOOGLE_user_type"},*/
	};

	bool convertStrToAttributeSemantic(const char* str, AttributeSemantic& semanticOut)
	{
		if (!str) return false;

		std::string string(str);

		auto tryToParseSemantic = [](const std::string& str, const char* strToFind, AttributeSemanticName semantic, AttributeSemantic& semanticOut) -> bool
		{
			size_t pos = str.find(strToFind);
			if (pos == std::string::npos) return false;

			size_t stringLength = strlen(strToFind);
			uint16_t index = 0;

			if (str.length() > stringLength)
			{
				index = stoi(str.substr(stringLength, stringLength - str.length()));
			}

			semanticOut.set(semantic, index);
			return true;
		};

		size_t prefix = string.find("in.var.");
		if (prefix != std::string::npos)
		{
			string.erase(prefix, 7);
		}

		if (tryToParseSemantic(string, "POSITION", AttributeSemanticName::POSITION, semanticOut)) return true;
		if (tryToParseSemantic(string, "COLOR", AttributeSemanticName::COLOR, semanticOut)) return true;
		if (tryToParseSemantic(string, "NORMAL", AttributeSemanticName::NORMAL, semanticOut)) return true;
		if (tryToParseSemantic(string, "TANGENT", AttributeSemanticName::TANGENT, semanticOut)) return true;
		if (tryToParseSemantic(string, "TEXCOORD", AttributeSemanticName::TEXCOORD, semanticOut)) return true;

		

		return false;
	}

	DescriptorType convertToYaptDescriptorType(SpvReflectDescriptorType spvType)
	{
		switch (spvType)
		{
		case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLER:
			return DescriptorType::SAMPLER;
		
		case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
			return DescriptorType::TEXTURE;
		case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_IMAGE:
			return DescriptorType::STORAGE_TEXTURE;
		case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
			return DescriptorType::UNIFORM_TEXEL_BUFFER;
		case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
			return DescriptorType::STORAGE_TEXEL_BUFFER;
		case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
			return DescriptorType::UNIFORM_BUFFER;
		case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER:
			return DescriptorType::STORAGE_BUFFER;
		case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
			return DescriptorType::UNIFORM_BUFFER_DYNAMIC;
		case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
			return DescriptorType::STORAGE_BUFFER_DYNAMIC;
		case SPV_REFLECT_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR:
			return DescriptorType::ACCELERATION_STRUCTURE;

		case SPV_REFLECT_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:
		case SPV_REFLECT_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
		default:
			{
				assert(!"DESCRIPTOR TYPE NOT SUPPORTED");
				return DescriptorType::TEXTURE;
			}
			
		}
	}


	bool ShaderModuleVk::compileFromHLSL(VkDevice device, const char* filepath, ShaderModuleType moduleType, const char* entryPoint, const ShaderModuleDefine* defines, size_t defineCount)
	{
		std::wstring profileWStr;

		std::wstring filePathWStr;
		std::wstring entryPointWStr;

		m_moduleType = moduleType;

		shaderTypeToProfile(moduleType, profileWStr);
		stringToWString(filepath, filePathWStr);
		stringToWString(entryPoint, entryPointWStr);

		std::vector<DxcDefine> definesDxc;
		std::vector<std::wstring> defineWstr;
		size_t defineCountFinal = defineCount + 1;

		definesDxc.resize(defineCountFinal);
		defineWstr.resize(defineCount * 2);

		for (size_t i = 0; i < defineCount; ++i)
		{
			stringToWString(defines[i].name, defineWstr[i * 2]);

			definesDxc[i].Name = defineWstr[i * 2].c_str();
			if (defines[i].value != nullptr)
			{
				stringToWString(defines[i].value, defineWstr[i * 2 + 1]);
				definesDxc[i].Value = defineWstr[i * 2 + 1].c_str();
			}
		}

		definesDxc.back().Name = L"VK";
		definesDxc.back().Value = L"1";

		RCPtr<IDxcBlob> shaderBlob = compileFromFile(profileWStr.data(), entryPointWStr.data(), moduleType == ShaderModuleType::LIBRARY_MODULE ? L"" : entryPointWStr.data(), filePathWStr.data(), definesDxc.data(), (UINT32)defineCountFinal, extraCompilerParams, countOf(extraCompilerParams));
		if (shaderBlob.get() && shaderBlob->GetBufferSize() > 0)
		{
			char* ptr = static_cast<char*>(shaderBlob->GetBufferPointer());
			m_spirv.assign(ptr, ptr + shaderBlob->GetBufferSize());
		}
		
		if (m_spirv.size() == 0) return false;

		bool success = reflect();
		if (success)
		{
			
			VkShaderModuleCreateInfo shdModuleInfo{};
			shdModuleInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
			shdModuleInfo.pNext = NULL;
			shdModuleInfo.pCode = (uint32_t*)m_spirv.data();
			shdModuleInfo.codeSize = m_spirv.size();
			shdModuleInfo.flags = 0;

			VkResult res = vkCreateShaderModule(device, &shdModuleInfo, VK_ALLOC_CB, &m_vkShaderModule);
			success = res == VK_SUCCESS;;
		}

		return success;
	}

	bool ShaderModuleVk::reflect()
	{
		SpvReflectShaderModule module;
		SpvReflectResult result = spvReflectCreateShaderModule(m_spirv.size(), m_spirv.data(), &module);
		if (result != SPV_REFLECT_RESULT_SUCCESS) return false;

		std::vector<SpvReflectInterfaceVariable*> inputVariables;
		std::vector<SpvReflectDescriptorSet*> descSets;
		std::vector<SpvReflectDescriptorBinding*> descBindings;
		// input variables
		if(result == SPV_REFLECT_RESULT_SUCCESS && m_moduleType == ShaderModuleType::VERTEX_MODULE)
		{
			uint32_t inputVariableCount = 0;

			result = spvReflectEnumerateInputVariables(&module, &inputVariableCount, NULL);
			if (result == SPV_REFLECT_RESULT_SUCCESS)
			{
				inputVariables.resize(inputVariableCount);
				result = spvReflectEnumerateInputVariables(&module, &inputVariableCount, inputVariables.data());

			}
		}

		if (result == SPV_REFLECT_RESULT_SUCCESS)
		{
			uint32_t descSetCount = 0;

			result = spvReflectEnumerateDescriptorSets(&module, &descSetCount, NULL);
			if (result == SPV_REFLECT_RESULT_SUCCESS)
			{
				descSets.resize(descSetCount);
				result = spvReflectEnumerateDescriptorSets(&module, &descSetCount, descSets.data());

			}
		}

		if (result == SPV_REFLECT_RESULT_SUCCESS)
		{
			uint32_t descBindingsCount = 0;

			result = spvReflectEnumerateDescriptorBindings(&module, &descBindingsCount, NULL);
			if (result == SPV_REFLECT_RESULT_SUCCESS)
			{
				descBindings.resize(descBindingsCount);
				result = spvReflectEnumerateDescriptorBindings(&module, &descBindingsCount, descBindings.data());

			}
		}

		if (result == SPV_REFLECT_RESULT_SUCCESS)
		{
			//handle input attributes
			for (size_t i = 0; i < inputVariables.size(); ++i)
			{
				SpvReflectFormat format = inputVariables[i]->format;
				const char* nameStr = inputVariables[i]->name;
				uint32_t location = inputVariables[i]->location;

				AttributeSemantic semantic;
				bool convStatus = convertStrToAttributeSemantic(nameStr, semantic);
				assert(convStatus);

				m_inputAttributes.push_back({ semantic, location });
			}

			std::sort(m_inputAttributes.data(), m_inputAttributes.data() + m_inputAttributes.size(), 
				[](const InputAttributes& a, const InputAttributes& b)
				{
					return a.location <= b.location;
				}
			);



			//handle descSets
			if (descSets.size() > 0)
			{
				uint32_t highestDescSetIndex = 0;
				for (size_t i = 0; i < descSets.size(); ++i)
				{
					highestDescSetIndex = max(descSets[i]->set % BINDINGPOINT_ALIAS_MULTIPLE, highestDescSetIndex);
				}

				m_sortedBindings.resize(highestDescSetIndex + 1);

				for (size_t i = 0; i < descSets.size(); ++i)
				{
					std::vector<ResourceBinding>& bindings = m_sortedBindings[i];
					SpvReflectDescriptorSet& descSetRefl = *descSets[i];

					bindings.resize(descSetRefl.binding_count);

					for (size_t k = 0; k < bindings.size(); ++k)
					{
						SpvReflectDescriptorBinding* bindingRefl = descSetRefl.bindings[k];
						ResourceBinding& binding = bindings[k];

						binding.name = bindingRefl->name;
						binding.type = convertToYaptDescriptorType(bindingRefl->descriptor_type);
						binding.descriptorCount = bindingRefl->count;
						binding.bindingIndex = bindingRefl->binding;
						binding.accessFlags = bindingRefl->resource_type == SPV_REFLECT_RESOURCE_FLAG_UAV ? AccessFlagsBits::ACCESS_FLAGS_READ_WRITE : AccessFlagsBits::ACCESS_FLAGS_READ;
					}

					std::sort(bindings.data(), bindings.data() + bindings.size(), [](const ResourceBinding& a, const ResourceBinding& b)
						{
							return a.bindingIndex < b.bindingIndex;
						});
				}
			}
			
		}

		spvReflectDestroyShaderModule(&module);

		return result == SPV_REFLECT_RESULT_SUCCESS;
	}
}