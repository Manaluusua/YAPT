#pragma once

#include <Gfx/GfxTypes.h>
#include <Renderer/Shared/MeshInternal.h>
namespace YAPT
{
	class MeshToVertexInputLayoutMappingUtility
	{
	public:

		bool generateMapping(const MeshLayoutInfo& info, AttributeSemantic* attributesToMap, size_t numberOfAttributes);

		const VertexBufferDefinition* getVertexBufferDefinitions() const { return m_vertexBufferDefs.data(); }
		size_t getNumberOfVertexBufferDefinitions() const { return m_vertexBufferDefs.size(); }
		size_t getBufferIndexForDefinitionAtIndex(size_t index) const { assert(index < m_vertexBufferDefinitionToBufferIndexMapping.size()); return m_vertexBufferDefinitionToBufferIndexMapping[index]; }

	private:
		std::vector<size_t> m_vertexBufferDefinitionToBufferIndexMapping;
		std::vector<VertexBufferDefinition> m_vertexBufferDefs;
		std::vector<VertexInputAttribute> m_attributes;
	};

}