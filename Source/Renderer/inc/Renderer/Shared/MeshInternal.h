#pragma once

#include <Renderer/Shared/BufferImpl.h>
#include <Common/RCObjectPtr.h>
#include <vector>
#include <Gfx/GfxApi.h>
#include <Renderer/Mesh.h>
#include <Common/BubbleArray.h>
namespace YAPT
{
	typedef size_t MeshIndex;
	constexpr MeshIndex InvalidMeshIndex = InvalidBubbleArrayIndex;

	typedef uint64_t MeshLayoutID;
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
		MeshInternal(MeshIndex id, GfxApiHandle gfx, MeshLayoutID layoutID, const MeshLayoutInfo& data, size_t submeshCount, bool use16BitIndices);

		void setVertexBuffer(size_t bufferIndex, BufferImpl* buffer, size_t offsetInBytes);
		void setIndexBuffer(BufferImpl* buffer, size_t offsetInBytes);

		const MeshBufferBinding& getVertexBuffer(size_t index) const;
		const MeshBufferBinding& getIndexBuffer() const;

		const MeshLayoutInfo& getLayoutInfo() const;

		size_t getSubmeshCount() const { return m_SubmeshRanges.size(); }
		const SubmeshDefinition& getSubmesh(size_t subMeshIndex) const
		{ 
			assert(subMeshIndex < getSubmeshCount());
			return m_SubmeshRanges[subMeshIndex];
		}
		void setSubmesh(size_t subMeshIndex, const SubmeshDefinition& range)
		{ 
			assert(subMeshIndex < getSubmeshCount());
			m_SubmeshRanges[subMeshIndex] = range;
		}

		MeshLayoutID getMeshLayoutID() const { return m_meshLayoutId; }

		ResourceFormat getIndexBufferFormat() const { return m_indexFormat; }
		MeshIndex getID() const { return m_id; }

	private:
		
		GfxApiHandle m_gfx;
		MeshLayoutInfo m_info;
		std::vector<MeshBufferBinding> m_vertexBuffers;
		MeshBufferBinding m_indexBuffer;
		std::vector<SubmeshDefinition> m_SubmeshRanges;
		MeshLayoutID m_meshLayoutId;
		ResourceFormat m_indexFormat;
		const MeshIndex m_id;
	};
}

