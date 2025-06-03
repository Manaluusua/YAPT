#include <PyMesh.h>
#include <Renderer/Mesh.h>
#include <Renderer/Renderer.h>
#include <pybind11/stl.h>
#include <PyBuffer.h>
namespace YAPT
{
	DEFINE_BINDING_CLASS(PyMesh);
	PyMesh::PyMesh(Renderer* rend, const char* name, const std::vector<PyVertexBufferLayout>& layouts, size_t vertexCount, size_t submeshCount, bool use16BitIndices)
		:m_name(name)
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

		m_mesh = rend->createMesh(layoutsNative.data(), layoutsNative.size(), vertexCount, submeshCount, use16BitIndices);
		m_mesh.get()->Release();

	}
	PyMesh::~PyMesh()
	{
		m_mesh = nullptr;
	}

	const char* PyMesh::getName() const
	{
		return m_name.c_str();
	}

	void PyMesh::setVertexBuffer(size_t bufferIndex, PyBuffer* buffer, size_t offsetInBytes)
	{
		m_mesh->setVertexBuffer(bufferIndex, buffer->getBuffer().get(), offsetInBytes);
	}
	void PyMesh::setIndexBuffer(PyBuffer* buffer, size_t offsetInBytes)
	{
		m_mesh->setIndexBuffer(buffer->getBuffer().get(), offsetInBytes);
	}

	size_t PyMesh::getSubmeshCount() const
	{
		return m_mesh->getSubmeshCount();
	}

	void PyMesh::setSubmesh(size_t submeshIndex, size_t indexOffset, size_t indexCount, size_t vertexOffset, const vec3p& min, const vec3p& max)
	{
		SubmeshDefinition smRange;
		smRange.indexOffset = indexOffset;
		smRange.indexCount = indexCount;
		smRange.vertexOffset = vertexOffset;
		smRange.bounds.min = min;
		smRange.bounds.max = max;
		m_mesh->setSubmesh(submeshIndex, smRange);
	}

	BINDING_FUNC(PyMesh, m)
	{
		pybind11::class_<PyMeshAttribute>(m, "MeshAttribute")
			.def(pybind11::init<>())  // Default constructor
			.def(pybind11::init<ResourceFormat, AttributeSemanticName, int>())
			.def_readwrite("format", &PyMeshAttribute::format)
			.def_readwrite("semanticName", &PyMeshAttribute::semanticName)
			.def_readwrite("semanticIndex", &PyMeshAttribute::semanticIndex);

		pybind11::class_<PyVertexBufferLayout>(m, "VertexBufferLayout")
			.def(pybind11::init<>())  // Default constructor
			.def(pybind11::init<const std::vector<PyMeshAttribute>&, uint32_t>())
			.def_readwrite("attributes", &PyVertexBufferLayout::attributes)
			.def_readwrite("vertexStrideInBytes", &PyVertexBufferLayout::vertexStrideInBytes)
			.def_readonly_static("STRIDE_TIGHTLY_PACKED", &PyVertexBufferLayout::STRIDE_TIGHTLY_PACKED);


		pybind11::class_<PyMesh>(m, "Mesh")
			.def("getName", &PyMesh::getName)
			.def("setVertexBuffer", &PyMesh::setVertexBuffer)
			.def("setIndexBuffer", &PyMesh::setIndexBuffer)
			.def("setSubmesh", &PyMesh::setSubmesh)
			.def("getSubmeshCount", &PyMesh::getSubmeshCount);

		
	}
}