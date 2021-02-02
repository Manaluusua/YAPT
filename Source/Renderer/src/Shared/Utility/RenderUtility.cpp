#include <Renderer/Shared/Utility/RenderUtility.h>
#include <algorithm>
namespace YAPT
{
	//MeshToVertexInputLayoutMappingUtility Impl
	bool MeshToVertexInputLayoutMappingUtility::generateMapping(const MeshLayoutInfo& info, AttributeSemantic* attributesToMap, size_t numberOfAttributes)
	{
		struct FoundMapping
		{
			size_t bufferIndex;
			size_t attributeIndex;
		};

		std::vector<FoundMapping> mappings;
		mappings.reserve(numberOfAttributes);

		for (size_t attributeIndex = 0; attributeIndex < numberOfAttributes; ++attributeIndex)
		{
			bool matchFound = false;
			for (size_t configIndex = 0; (configIndex < info.vertexBufferConfigurations.size()) && !matchFound; ++configIndex)
			{
				const VertexBufferConfiguration& config = info.vertexBufferConfigurations[configIndex];
				for (size_t i = 0; (i < config.attributes.size()) && !matchFound; ++i)
				{
					if (attributesToMap[attributeIndex] == config.attributes[i].semantic)
					{
						mappings.push_back({ configIndex, i});
						matchFound = true;
					}
				}
			}
		}

		if (mappings.size() != numberOfAttributes) return false;

		m_attributes.resize(numberOfAttributes);

		m_vertexBufferDefs.resize(info.vertexBufferConfigurations.size());
		for (size_t i = 0; i < m_vertexBufferDefs.size(); ++i)
		{
			m_vertexBufferDefs[i].numberOfVertexAttributes = 0;
			m_vertexBufferDefs[i].vertexInputRate = 0;
			m_vertexBufferDefs[i].stride = info.vertexBufferConfigurations[i].stride;
		}


		std::sort(mappings.data(), mappings.data() + mappings.size(), [](const FoundMapping& a, const FoundMapping& b)
			{
				if (a.bufferIndex == b.bufferIndex)
				{
					return a.attributeIndex < b.attributeIndex;
				}
				else
				{
					return a.bufferIndex < b.bufferIndex;
				}
			});

		size_t currentBufferIndex = 0;
		m_vertexBufferDefs[0].attributes = m_attributes.data();
		for (size_t i = 0; i < mappings.size(); ++i)
		{
			const FoundMapping& mapping = mappings[i];
			if (mapping.bufferIndex != currentBufferIndex)
			{
				m_vertexBufferDefs[mapping.bufferIndex].attributes = &m_attributes[i];
				currentBufferIndex = mapping.bufferIndex;
			}
			
			m_vertexBufferDefs[mapping.bufferIndex].numberOfVertexAttributes += 1;
			VertexInputAttribute& attrib = m_attributes[i];
			attrib.format = info.vertexBufferConfigurations[mapping.bufferIndex].attributes[mapping.attributeIndex].format;
			attrib.perVertexOffset = info.vertexBufferConfigurations[mapping.bufferIndex].offsetFromVertexStart[mapping.attributeIndex];
			attrib.shaderInputSlot = info.vertexBufferConfigurations[mapping.bufferIndex].attributes[mapping.attributeIndex].semantic;
		}

		return true;
	}
}