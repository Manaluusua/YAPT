#ifndef YAPT_LIGHT_H
#define YAPT_LIGHT_H

#include <Common/RCObject.h>
#include <Math/Math.h>

namespace YAPT
{
	enum class LightType : uint32_t
	{
		POINT,
		SPOT,
		DIRECTIONAL,
	};

	/**
		Infinitesimal (punctual) light source. Point and spot lights are located at the transform position, spot and directional lights
		emit towards the transform forward axis. Color * intensity is radiant intensity for point and spot lights, irradiance for directional lights.
		Spot cone angles are half angles in radians.
	*/
	class Light : public RCObject
	{
	public:
		virtual void setType(LightType type) = 0;
		virtual LightType getType() const = 0;

		virtual void setTransform(const mat4& transform) = 0;

		virtual void setColor(const vec3p& color) = 0;
		virtual const vec3p& getColor() const = 0;

		virtual void setIntensity(float intensity) = 0;
		virtual float getIntensity() const = 0;

		virtual void setSpotConeAngles(float innerAngle, float outerAngle) = 0;
		virtual float getSpotInnerAngle() const = 0;
		virtual float getSpotOuterAngle() const = 0;

	protected:
		virtual ~Light() {}
	};
}

#endif
