#pragma once
#include <PyBindingsCommon.h>
#include <Gfx/GfxTypes.h>
#include <Renderer/Mesh.h>

namespace YAPT
{
	struct PyMeshAttribute
	{
		ResourceFormat format;
		AttributeSemanticName semanticName;
		uint32_t semanticIndex;
	};

	struct PyVertexBufferLayout
	{
		std::vector<PyMeshAttribute> attributes;
		uint32_t vertexStrideInBytes;
		static constexpr uint32_t STRIDE_TIGHTLY_PACKED = VERTEX_STRIDE_TIGHTLY_PACKED;
	};

	class Renderer;
	class PyMesh
	{
	public:
		DECLARE_BINDING_CLASS(PyRenderer);

		PyMesh(Renderer* rend, const std::vector<PyVertexBufferLayout>& layouts, size_t vertexCount);
		~PyMesh();

	private:
		RCObjectPtr<Mesh> m_mesh;
	};
}