#include <PyRenderObject.h>

#include <PyMesh.h>
#include <PyMaterial.h>


namespace YAPT
{
	DEFINE_BINDING_CLASS(PyRenderObject);

	PyRenderObject::PyRenderObject(const char* name, Scene* scene)
		:m_name(name),
		m_obj(scene->createModel()),
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

	void PyRenderObject::setMesh(PyMesh* mesh)
	{
		m_obj->setMesh(mesh->getMesh());
	}

	void PyRenderObject::setMaterials(std::vector<PyMaterial*> materials)
	{
		std::vector<Material*> mat;
		mat.resize(materials.size());
		for (size_t i = 0; i < materials.size(); ++i)
		{
			mat[i] = materials[i]->getMaterial();
		}

		m_obj->setMaterials(mat.data(), mat.size());
	}

	void PyRenderObject::setMaterial(PyMaterial* material, size_t materialIndex)
	{
		m_obj->setMaterial(material->getMaterial(), materialIndex);
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
			.def("getTransform", &PyRenderObject::getTransform, pybind11::return_value_policy::reference_internal);
	}
}