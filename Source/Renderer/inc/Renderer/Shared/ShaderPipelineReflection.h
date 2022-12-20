#pragma once

#include <Renderer/Shared/GfxTypes.h>
#include <vector>
#include <unordered_map>

#define SHADERTABLE_EXTRADATA_NAME "___ShaderTableExtraData___"

namespace YAPT
{
	

	class ShaderPipelineReflection
	{
	public:
		
		struct ResourceBinding
		{
			uint32_t bindingIndex = uint32_t(-1);
			uint32_t descriptorCount = 0;
			DescriptorType type = DescriptorType::SAMPLER;
			AccessFlags accessFlags = 0;
			ShaderStages shaderStages = 0;
		};

		struct NameMapping
		{
			uint32_t descSetIndex;
			uint32_t bindingPoint;
		};

		ShaderPipelineReflection();
		virtual ~ShaderPipelineReflection();

		size_t getHighestDescSetIndex() const;

		size_t getNumberOfBindingDefinitionsForDescSet(size_t index) const;

		const ResourceBinding* getBindingDefinitionsForDescSet(size_t index) const;

		bool getNameMapping(ShaderModuleType type, const char* resourceName, NameMapping& mapping) const;

		bool hasShaderTableBuffer() const { return m_hasShaderTableBuffer; }

		size_t getNumberOfInputAttributes() const { return m_inputAttributes.size(); }

		const AttributeSemantic* getInputAttributes() const { return m_inputAttributes.data(); }


	protected:

		struct DescriptorSetReflection
		{
			std::vector<ResourceBinding> bindingDefinitions;
		};

		struct ShaderTableBufferEntry
		{
			uint32_t descSetIndex;
			uint32_t bindingPoint;
		};

		std::vector<DescriptorSetReflection> m_reflectionDataPerDescSet;
		std::unordered_map<std::string, NameMapping> m_nameMappings[(size_t)ShaderModuleType::LAST];
		std::vector<AttributeSemantic> m_inputAttributes;
		bool m_hasShaderTableBuffer;
	};

}