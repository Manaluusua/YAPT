#ifndef YAPT_MESH_H
#define YAPT_MESH_H

#include <Common/RCObject.h>
#include <Renderer/RendererCommonTypes.h>

namespace YAPT
{
	constexpr uint32_t VERTEX_STRIDE_TIGHTLY_PACKED = uint32_t(-1);

	struct VertexBufferLayout
	{
		Attribute* attributes;
		uint32_t attributeCount;
		uint32_t vertexStrideInBytes = VERTEX_STRIDE_TIGHTLY_PACKED;
	};


	class Buffer;
	class Mesh : public RCObject 
	{
	public:

		virtual void setVertexBuffer(size_t bufferIndex, Buffer* buffer, size_t offsetInBytes) = 0;
		virtual void setIndexBuffer(Buffer* buffer, size_t offsetInBytes, size_t numberOfPrimitives) = 0;
	protected:
		virtual ~Mesh() {}
	};
}

#endif