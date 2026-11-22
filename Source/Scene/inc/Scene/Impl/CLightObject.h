#pragma once

#include <Scene/LightObject.h>
#include <Common/RCObjectPtr.h>

namespace YAPT
{
	class CScene;
	class Renderer;

	class CLightObject : public LightObject
	{
	public:
		CLightObject(Renderer* renderer, CScene* scene, LightType type);
		~CLightObject();

		virtual void setType(LightType type) final { m_light->setType(type); }
		virtual LightType getType() const final { return m_light->getType(); }

		virtual void setColor(const vec3p& color) final { m_light->setColor(color); }
		virtual const vec3p& getColor() const final { return m_light->getColor(); }

		virtual void setIntensity(float intensity) final { m_light->setIntensity(intensity); }
		virtual float getIntensity() const final { return m_light->getIntensity(); }

		virtual void setSpotConeAngles(float innerAngle, float outerAngle) final { m_light->setSpotConeAngles(innerAngle, outerAngle); }
		virtual float getSpotInnerAngle() const final { return m_light->getSpotInnerAngle(); }
		virtual float getSpotOuterAngle() const final { return m_light->getSpotOuterAngle(); }

		void refresh();

	private:
		static void transformChanged(void* ptr);

		CScene* m_scene;
		RCObjectPtr<Light> m_light;
		bool m_pendingRefresh;
	};
}
