#pragma once

#include <Renderer/Light.h>
#include <Gfx/GfxBasicTypes.h>

namespace YAPT
{
	class LightManager;

	class LightProxy : public Light
	{
		friend class LightManager;
	public:

		enum LightState
		{
			LIGHTSTATE_NOCHANGES = 0,
			LIGHTSTATE_CREATED = YAPTBIT(1),
			LIGHTSTATE_MODIFIED = YAPTBIT(2),
			LIGHTSTATE_DESTROYED = YAPTBIT(3),
		};

		LightProxy(LightManager* mngr, LightType type);
		virtual ~LightProxy();

		virtual void setType(LightType type) final;
		virtual LightType getType() const final { return m_type; }

		virtual void setTransform(const mat4& transform) final;

		virtual void setColor(const vec3p& color) final;
		virtual const vec3p& getColor() const final { return m_color; }

		virtual void setIntensity(float intensity) final;
		virtual float getIntensity() const final { return m_intensity; }

		virtual void setSpotConeAngles(float innerAngle, float outerAngle) final;
		virtual float getSpotInnerAngle() const final { return m_spotInnerAngle; }
		virtual float getSpotOuterAngle() const final { return m_spotOuterAngle; }

	protected:
		virtual void allReferencesReleased() final;

	private:
		void setDirty();

		LightType m_type;
		mat4 m_transform;
		vec3p m_color;
		float m_intensity;
		float m_spotInnerAngle;
		float m_spotOuterAngle;

		LightManager* m_mngr;

		size_t _lightState;
		size_t _id;
	};
}
