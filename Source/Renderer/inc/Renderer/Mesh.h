#ifndef YAPT_MESH_H
#define YAPT_MESH_H

#include <Common/RCObject.h>
#include <Gfx/GfxBasicTypes.h>

namespace YAPT
{
	constexpr uint32_t VERTEX_STRIDE_TIGHTLY_PACKED = uint32_t(-1);

	struct VertexBufferLayout
	{
		Attribute* attributes;
		uint32_t attributeCount;
		uint32_t vertexStrideInBytes = VERTEX_STRIDE_TIGHTLY_PACKED;
	};

	struct SubmeshRange
	{
		SubmeshRange()
			:indexOffset(0),
			indexCount(0)
		{}
		SubmeshRange(size_t offset, size_t count)
			:indexOffset(offset),
			indexCount(count)
		{}
		size_t indexOffset;
		size_t indexCount;
	};

	class Buffer;
	class Mesh : public RCObject 
	{
	public:
		virtual void setSubmesh(size_t submeshIndex, const SubmeshRange& range) = 0;
		virtual void setVertexBuffer(size_t bufferIndex, Buffer* buffer, size_t offsetInBytes) = 0;
		virtual void setIndexBuffer(Buffer* buffer, size_t offsetInBytes) = 0;
	protected:
		virtual ~Mesh() {}
	};
}

#endif