#pragma once

#include "SceneObject.h"
#include <Renderer/Light.h>

namespace YAPT
{
	//punctual light placed by its transform: point and spot lights sit at the position, spot and directional lights point along the forward axis
	class LightObject : public SceneObject
	{
	public:
		virtual void setType(LightType type) = 0;
		virtual LightType getType() const = 0;

		virtual void setColor(const vec3p& color) = 0;
		virtual const vec3p& getColor() const = 0;

		virtual void setIntensity(float intensity) = 0;
		virtual float getIntensity() const = 0;

		virtual void setSpotConeAngles(float innerAngle, float outerAngle) = 0;
		virtual float getSpotInnerAngle() const = 0;
		virtual float getSpotOuterAngle() const = 0;

		virtual ~LightObject() {}
	};
}
