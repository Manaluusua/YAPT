#include <PySceneObject.h>
#include <pybind11/stl.h>



namespace YAPT
{
	DEFINE_BINDING_CLASS(PySceneObject);

	PySceneObject::PySceneObject(const char* name, SceneObject* scene)
		:m_name(name),
		m_obj(scene),
		m_transform(&m_obj->getTransform())
	{
	}
	PySceneObject::~PySceneObject()
	{
		m_obj = nullptr;
	}

	const char* PySceneObject::getName() const
	{
		return m_name.c_str();
	}

	PyTransform* PySceneObject::getTransform()
	{
		return &m_transform;
	}

	BINDING_FUNC(PySceneObject, m)
	{
		pybind11::class_<PySceneObject>(m, "SceneObject")
			.def("getName", &PySceneObject::getName)
			.def("getTransform", &PySceneObject::getTransform, pybind11::return_value_policy::reference_internal);
	}
}