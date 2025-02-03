#include <Gfx/ShaderPipelineReflection.h>

namespace YAPT
{
	ShaderPipelineReflection::ShaderPipelineReflection()
	{
		m_hasShaderTableBuffer = false;

	}
	ShaderPipelineReflection::~ShaderPipelineReflection()
	{

	}

	size_t ShaderPipelineReflection::getHighestDescSetIndex() const
	{
		return m_reflectionDataPerDescSet.size() - 1;
	}

	size_t ShaderPipelineReflection::getNumberOfBindingDefinitionsForDescSet(size_t index) const
	{
		if (index > getHighestDescSetIndex()) return 0;
		return m_reflectionDataPerDescSet[index].bindingDefinitions.size();
	}

	const ShaderPipelineReflection::ResourceBinding* ShaderPipelineReflection::getBindingDefinitionsForDescSet(size_t index) const
	{
		if (index > getHighestDescSetIndex()) return nullptr;
		return m_reflectionDataPerDescSet[index].bindingDefinitions.data();
	}

	bool ShaderPipelineReflection::getNameMapping(ShaderModuleType type, const char* resourceName, NameMapping& mapping) const
	{
		size_t ind = (size_t)type;
		auto iter = m_nameMappings[ind].find(resourceName);
		if (iter != m_nameMappings[ind].end())
		{
			mapping = iter->second;
			return true;
		}
		return false;
	}



}