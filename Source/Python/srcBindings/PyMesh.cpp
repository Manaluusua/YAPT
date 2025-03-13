#include <PyMesh.h>
#include <Renderer/Mesh.h>
#include <Renderer/Renderer.h>
namespace YAPT
{
	DEFINE_BINDING_CLASS(PyMesh);
	PyMesh::PyMesh(Renderer* rend, const std::vector<PyVertexBufferLayout>& layouts, size_t vertexCount)
	{
		std::vector<VertexBufferLayout> layoutsNative;
		std::vector<Attribute> attributesNative;

		size_t attributeCount = 0;
		for (size_t i = 0; i < layouts.size(); ++i)
		{
			attributeCount += layouts[i].attributes.size();
		}
		
		layoutsNative.reserve(layouts.size());
		attributesNative.reserve(attributeCount);
		
		for (size_t i = 0; i < layouts.size(); ++i)
		{
			VertexBufferLayout dl;
			const PyVertexBufferLayout& sl = layouts[i];
			dl.attributeCount = (uint32_t)sl.attributes.size();
			dl.attributes = attributesNative.data() + attributesNative.size();
			dl.vertexStrideInBytes = sl.vertexStrideInBytes;

			for (size_t k = 0; k < sl.attributes.size(); ++k)
			{
				Attribute attrib;
				attrib.format = sl.attributes[k].format;
				attrib.semantic = AttributeSemantic(sl.attributes[k].semanticName, sl.attributes[k].semanticIndex);
				attributesNative.push_back(attrib);
			}
			layoutsNative.push_back(dl);

		}

		m_mesh = rend->createMesh(layoutsNative.data(), layoutsNative.size(), vertexCount);

	}
	PyMesh::~PyMesh()
	{
		m_mesh = nullptr;
	}


	BINDING_FUNC(PyMesh, m)
	{
		pybind11::class_<PyMeshAttribute>(m, "PyMeshAttribute")
			.def(pybind11::init<>())  // Default constructor
			.def_readwrite("format", &PyMeshAttribute::format)
			.def_readwrite("semanticName", &PyMeshAttribute::semanticName)
			.def_readwrite("semanticIndex", &PyMeshAttribute::semanticIndex);

		pybind11::class_<PyVertexBufferLayout>(m, "PyVertexBufferLayout")
			.def(pybind11::init<>())  // Default constructor
			.def_readwrite("attributes", &PyVertexBufferLayout::attributes)
			.def_readwrite("vertexStrideInBytes", &PyVertexBufferLayout::vertexStrideInBytes)
			.def_readonly_static("STRIDE_TIGHTLY_PACKED", &PyVertexBufferLayout::STRIDE_TIGHTLY_PACKED);

		
	}
}