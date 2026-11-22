#include <PyLight.h>

namespace YAPT
{
	DEFINE_BINDING_CLASS_WITH_BASE(PyLight, PySceneObject);

	PyLight::PyLight(const char* name, LightObject* obj)
		:PySceneObject(name, obj)
	{

	}
	PyLight::~PyLight()
	{

	}

	BINDING_FUNC(PyLight, m)
	{
		auto light = pybind11::class_<PyLight, PySceneObject>(m, "Light");
		light.def("setType", &PyLight::setType)
			.def("getType", &PyLight::getType)
			.def("setColor", &PyLight::setColor)
			.def("getColor", &PyLight::getColor, pybind11::return_value_policy::reference_internal)
			.def("setIntensity", &PyLight::setIntensity)
			.def("getIntensity", &PyLight::getIntensity)
			.def("setSpotConeAngles", &PyLight::setSpotConeAngles)
			.def("getSpotInnerAngle", &PyLight::getSpotInnerAngle)
			.def("getSpotOuterAngle", &PyLight::getSpotOuterAngle);

		pybind11::enum_<LightType>(light, "LightType")
			.value("POINT", LightType::POINT)
			.value("SPOT", LightType::SPOT)
			.value("DIRECTIONAL", LightType::DIRECTIONAL)
			.export_values();
	}
}
