#pragma once
#include <PyBindingsCommon.h>
#include <Scene/LightObject.h>
#include <PySceneObject.h>

namespace YAPT
{
	class PyLight : public PySceneObject
	{
	public:
		DECLARE_BINDING_CLASS(PyLight);

		PyLight(const char* name, LightObject* obj);
		~PyLight();

		void setType(LightType type) { getLight()->setType(type); }
		LightType getType() const { return getLight()->getType(); }

		void setColor(const vec3p& color) { getLight()->setColor(color); }
		const vec3p& getColor() const { return getLight()->getColor(); }

		void setIntensity(float intensity) { getLight()->setIntensity(intensity); }
		float getIntensity() const { return getLight()->getIntensity(); }

		void setSpotConeAngles(float innerAngle, float outerAngle) { getLight()->setSpotConeAngles(innerAngle, outerAngle); }
		float getSpotInnerAngle() const { return getLight()->getSpotInnerAngle(); }
		float getSpotOuterAngle() const { return getLight()->getSpotOuterAngle(); }

	private:
		LightObject* getLight() const { return static_cast<LightObject*>(getWrappedObject()); }
	};
}
