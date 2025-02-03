#pragma once

#include <Renderer/Shared/BufferImpl.h>
#include <Common/RCObjectPtr.h>
#include <vector>
#include <Gfx/GfxApi.h>

namespace YAPT
{
	typedef size_t MeshLayoutID;
	struct VertexBufferConfiguration
	{
		std::vector<Attribute> attributes;
		std::vector<uint32_t> offsetFromVertexStart;
		uint32_t stride;
	};

	struct AttributeMapping
	{
		bool isValid() const
		{
			return bufferIndex != uint32_t(-1) && attributeIndex != uint32_t(-1);
		}

		uint32_t bufferIndex = uint32_t(-1);
		uint32_t attributeIndex = uint32_t(-1);
	};

	struct MeshLayoutInfo
	{
		std::vector<VertexBufferConfiguration> vertexBufferConfigurations;
		size_t numberOfVertices;

		AttributeMapping position0;
		AttributeMapping normal0;
		AttributeMapping tangent0;
		AttributeMapping uv0;
	};

	struct MeshBufferBinding
	{
		RCObjectPtr<BufferImpl> buffer;
		BufferViewHandle bufferView;
		size_t offsetInBytes;
	};

	class MeshInternal
	{
	public:
		MeshInternal(size_t id, GfxApiHandle gfx, MeshLayoutID layoutID, const MeshLayoutInfo& data, bool use16BitIndices = false);

		void setVertexBuffer(size_t bufferIndex, BufferImpl* buffer, size_t offsetInBytes);
		void setIndexBuffer(BufferImpl* buffer, size_t offsetInBytes);

		const MeshBufferBinding& getVertexBuffer(size_t index) const;
		const MeshBufferBinding& getIndexBuffer() const;

		const MeshLayoutInfo& getLayoutInfo() const;

		size_t getPrimitiveCount() const { return m_primitiveCount; }
		void setPrimitiveCount(size_t prim) { m_primitiveCount = prim; }

		size_t getMeshIndex() const { return m_id; }
		
		MeshLayoutID getMeshLayoutID() const { return m_meshLayoutId; }
	private:
		
		GfxApiHandle m_gfx;
		MeshLayoutInfo m_info;
		std::vector<MeshBufferBinding> m_vertexBuffers;
		MeshBufferBinding m_indexBuffer;
		size_t m_primitiveCount;
		MeshLayoutID m_meshLayoutId;
		ResourceFormat m_indexFormat;
		const size_t m_id;
	};
}

