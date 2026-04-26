#include <PyTransform.h>
#include <Scene/Transform.h>
namespace YAPT
{
	DEFINE_BINDING_CLASS(PyTransform);

	PyTransform::PyTransform(Transform* t)
		:m_transform(t)
	{

	}

	PyTransform::~PyTransform()
	{

	}

	void PyTransform::setTranslation(const vec3p& translation)
	{
		m_transform->setTranslation(translation);
	}
	void PyTransform::addTranslation(const vec3p& translation)
	{
		m_transform->addTranslation(translation);
	}
	const vec3p PyTransform::getTranslation() const
	{
		return m_transform->getTranslation();
	}

	void PyTransform::setScale(const vec3p& scale)
	{
		m_transform->setScale(scale);
	}
	void PyTransform::addScale(const vec3p& scale)
	{
		m_transform->addScale(scale);
	}
	const vec3p PyTransform::getScale() const
	{
		return m_transform->getScale();
	}

	void PyTransform::setOrientation(const vec4p& orientation)
	{
		m_transform->setOrientation((quat&)orientation);
	}
	void PyTransform::addOrientation(const vec4p& orientation)
	{
		m_transform->addOrientation((quat&)orientation);
	}
	const vec4p PyTransform::getOrientation() const
	{
		return (vec4p&)m_transform->getOrientation();
	}

	void PyTransform::lookAt(const vec3p& eye, const vec3p& at, const vec3p& up)
	{
		m_transform->lookAt(eye, at, up);
	}

	void PyTransform::setParent(PyTransform* transform)
	{
		m_transform->setParent(transform != nullptr ? transform->m_transform : nullptr);
	}

	vec3p PyTransform::right()
	{
		return m_transform->right();
	}
	vec3p PyTransform::up()
	{
		return m_transform->up();
	}
	vec3p PyTransform::forward()
	{
		return m_transform->forward();
	}

	BINDING_FUNC(PyTransform, m)
	{
		pybind11::class_<PyTransform>(m, "Transform")
			.def("setTranslation", &PyTransform::setTranslation)
			.def("addTranslation", &PyTransform::addTranslation)
			.def("getTranslation", &PyTransform::getTranslation, pybind11::return_value_policy::reference)

			.def("setScale", &PyTransform::setScale)
			.def("addScale", &PyTransform::addScale)
			.def("getScale", &PyTransform::getScale, pybind11::return_value_policy::reference)

			.def("setOrientation", &PyTransform::setOrientation)
			.def("addOrientation", &PyTransform::addOrientation)
			.def("getOrientation", &PyTransform::getOrientation, pybind11::return_value_policy::reference)

			.def("lookAt", &PyTransform::lookAt)

			.def("right", &PyTransform::right)
			.def("up", &PyTransform::up)
			.def("forward", &PyTransform::forward)
			.def("setParent", &PyTransform::setParent);
	}

}