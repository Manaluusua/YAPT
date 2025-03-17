#include <PyCamera.h>

namespace YAPT
{
	DEFINE_BINDING_CLASS(PyCamera)

	BINDING_FUNC(PyCamera, m)
	{
		auto cam = pybind11::class_<PyCamera>(m, "Camera");
        cam.def("setProjectionPerspective", &PyCamera::setProjectionPerspective)
            .def("setProjectionOrthographic", &PyCamera::setProjectionOrthographic)
            .def("setFOVY", &PyCamera::setFOVY)
            .def("setAspectRatio", &PyCamera::setAspectRatio)
            .def("setNearPlane", &PyCamera::setNearPlane)
            .def("setFarPlane", &PyCamera::setFarPlane)
            .def("setOrthographicSize", &PyCamera::setOrthographicSize)
            .def("setProjectionType", &PyCamera::setProjectionType)
            .def("getFOVY", &PyCamera::getFOVY)
            .def("getAspectRatio", &PyCamera::getAspectRatio)
            .def("getNearPlane", &PyCamera::getNearPlane)
            .def("getFarPlance", &PyCamera::getFarPlance)
            .def("getOrthographicSize", &PyCamera::getOrthographicSize)
            .def("getProjectionType", &PyCamera::getProjectionType)
            .def("getTransform", &PyCamera::getTransform, pybind11::return_value_policy::reference_internal);

		pybind11::enum_<Camera::ProjectionType>(cam, "ProjectionType")
			.value("PERSPECTIVE", Camera::ProjectionType::PERSPECTIVE)
			.value("ORTHOGRAPHIC", Camera::ProjectionType::ORTHOGRAPHIC)
			.export_values();
	}
}