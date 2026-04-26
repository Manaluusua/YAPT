#include <PyRenderObject.h>
#include <pybind11/stl.h>



namespace YAPT
{
	DEFINE_BINDING_CLASS(PyRenderObject);

	PyRenderObject::PyRenderObject(const char* name, Scene* scene)
		:m_name(name),
		m_obj(scene->createRenderableObject()),
		m_transform(&m_obj->getTransform())
	{
		m_obj->Release();
	}
	PyRenderObject::~PyRenderObject()
	{
		m_obj = nullptr;
	}

	const char* PyRenderObject::getName() const
	{
		return m_name.c_str();
	}

	void PyRenderObject::setMesh(std::shared_ptr<PyMesh> mesh)
	{
		m_mesh = mesh;
		m_obj->setMesh(m_mesh->getMesh());
	}

	void PyRenderObject::setMaterials(std::vector<std::shared_ptr<PyMaterial>> materials)
	{
		std::vector<Material*> mat;
		mat.resize(materials.size());
		for (size_t i = 0; i < materials.size(); ++i)
		{
			mat[i] = materials[i]->getMaterial();
		}

		m_obj->setMaterials(mat.data(), mat.size());
		m_materials = materials;
	}

	void PyRenderObject::setMaterial(std::shared_ptr<PyMaterial> material, size_t materialIndex)
	{
		m_obj->setMaterial(material->getMaterial(), materialIndex);

		if (m_materials.size() <= materialIndex)
		{
			m_materials.resize(materialIndex + 1, nullptr);
		}

		m_materials[materialIndex] = material;
	}

	std::vector<std::shared_ptr<PyMaterial>> PyRenderObject::getMaterials()
	{
		return m_materials;
	}
	std::shared_ptr<PyMesh> PyRenderObject::getMesh()
	{
		return m_mesh;
	}

	PyTransform* PyRenderObject::getTransform()
	{
		return &m_transform;
	}

	


	BINDING_FUNC(PyRenderObject, m)
	{
		pybind11::class_<PyRenderObject>(m, "RObject")
			.def("getName", &PyRenderObject::getName)
			.def("setMesh", &PyRenderObject::setMesh)
			.def("setMaterial", &PyRenderObject::setMaterial)
			.def("setMaterials", &PyRenderObject::setMaterials)
			.def("getMaterials", &PyRenderObject::getMaterials)
			.def("getMesh", &PyRenderObject::getMesh)
			.def("getTransform", &PyRenderObject::getTransform, pybind11::return_value_policy::reference_internal);
	}
}