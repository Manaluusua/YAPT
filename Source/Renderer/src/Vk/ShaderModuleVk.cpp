#include <Renderer/Vk/ShaderModuleVk.h>
#include <Renderer/Shared/Utility/DXCUtility.h>
#include <Common/CommonWindowsUtility.h>
#include <Common/CommonUtilities.h>
#include <spirv_reflect.h>
#include <assert.h>
namespace YAPT
{
	const static LPCWSTR extraCompilerParams[] =
	{
		{L"-spirv"},
		{L"-fspv-target-env=vulkan1.2"},
		{L"-fspv-extension=SPV_KHR_ray_tracing"},
		{L"-fspv-extension=SPV_EXT_descriptor_indexing"}
	};

	ShaderModuleVk::ShaderModuleVk()
	{
		
		
	}

	bool ShaderModuleVk::compileFromHLSL(const char* filepath, ShaderModuleType moduleType, const char* entryPoint, const ShaderModuleDefine* defines, size_t defineCount)
	{
		std::wstring profileWStr;

		std::wstring filePathWStr;
		std::wstring entryPointWStr;

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

		return reflect();
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
		if(result == SPV_REFLECT_RESULT_SUCCESS)
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

		


		// Output variables, descriptor bindings, descriptor sets, and push constants
		// can be enumerated and extracted using a similar mechanism.

		// Destroy the reflection data when no longer required.
		spvReflectDestroyShaderModule(&module);

		return false;
	}

	ShaderModuleVk::~ShaderModuleVk()
	{

	}
}