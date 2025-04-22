#pragma once
#include <PyBindingsCommon.h>
#include <Gfx/GfxTypes.h>
#include <Renderer/Mesh.h>
#include <Common/RCObjectPtr.h>

namespace YAPT
{
	struct PyMeshAttribute
	{
		PyMeshAttribute() = default;
		PyMeshAttribute(ResourceFormat f, AttributeSemanticName s, uint32_t si)
			:format(f),
			semanticName(s),
			semanticIndex(si)
		{

		}

		ResourceFormat format;
		AttributeSemanticName semanticName;
		uint32_t semanticIndex;
	};

	struct PyVertexBufferLayout
	{
		PyVertexBufferLayout() = default;

		PyVertexBufferLayout(const std::vector<PyMeshAttribute>& layouts, uint32_t stride)
			:attributes(layouts),
			vertexStrideInBytes(stride)
		{

		}
		std::vector<PyMeshAttribute> attributes;
		uint32_t vertexStrideInBytes;
		static constexpr uint32_t STRIDE_TIGHTLY_PACKED = VERTEX_STRIDE_TIGHTLY_PACKED;
	};

	class PyBuffer;
	class Renderer;
	class PyMesh
	{
	public:
		DECLARE_BINDING_CLASS(PyRenderer);

		PyMesh(Renderer* rend, const char* name, const std::vector<PyVertexBufferLayout>& layouts, size_t vertexCount, size_t submeshCount, bool use16BitIndices);
		
		void setVertexBuffer(size_t bufferIndex, PyBuffer* buffer, size_t offsetInBytes);
		void setIndexBuffer(PyBuffer* buffer, size_t offsetInBytes);
		void setSubmesh(size_t submeshIndex, size_t indexOffset, size_t indexCount, size_t vertexOffset);
		const char* getName() const;

		size_t getSubmeshCount() const;
		Mesh* getMesh() { return m_mesh; }

		~PyMesh();

	private:
		std::string m_name;
		RCObjectPtr<Mesh> m_mesh;
	};
}