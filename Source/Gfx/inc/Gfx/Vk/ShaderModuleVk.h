#pragma once
#include <Gfx/GfxTypes.h>


namespace YAPT
{

	struct ShaderModuleVk
	{
	public:

		struct ResourceBinding
		{
			std::string name;
			uint32_t bindingIndex;
			uint32_t descriptorCount;
			DescriptorType type;
			AccessFlags accessFlags;
		};

		struct InputAttributes
		{
			AttributeSemantic semantic;
			uint32_t location;
		};

		bool compileFromHLSL(VkDevice device, const char* filepath, ShaderModuleType moduleType, const char* entryPoint, const ShaderModuleDefine* defines, size_t defineCount);
		bool reflect();

		ShaderModuleType m_moduleType;
		std::vector<char> m_spirv;
		std::vector<std::vector<ResourceBinding>> m_sortedBindings;
		std::vector<InputAttributes> m_inputAttributes;
		VkShaderModule m_vkShaderModule;
	};
}