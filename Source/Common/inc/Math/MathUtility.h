#pragma once
#include "Math.h"

#define SAFE_DIVISOR(x) (((x) == 0.f) ? (1.f) : (x))


namespace YAPT
{
	namespace MathUtils
	{
		constexpr float PI = glm::pi<float>();

		inline float step(float y, float x)
		{
			return (x >= y) ? 1.f : 0.f;
		}

		inline float sqr(float v)
		{
			return v * v;
		}

		inline float saturate(float s)
		{
			return glm::clamp(s, 0.f, 1.f);
		}

		 
		inline vec3p sphericalToCartesian(float phi, float theta)
		{
			float cosTheta = glm::cos(theta);
			float sinTheta = sqrt(1.f - sqr(cosTheta));
			return vec3p(cos(phi) * sinTheta, cosTheta, sin(phi) * sinTheta);
		}

		void calculateArbitraryOrthogonalVector(const vec3p& vec, vec3p& orthogonalOut);

		void calculateTangentAndBitangentFromPositionAndUv(const vec3p& p1, const vec3p& p2, const vec3p& p3,
			const vec2p& t1, const vec2p& t2, const vec2p& t3, vec3p& outTangent, vec3p& outBitangent);

		void orthogonalizeAndNormalizeTangent(const vec3p& tangent, const vec3p& normal, vec3p& tangentOut);

		float calculateHandedness(const vec3p& tangent, const vec3p& bitangent, const vec3p& normal);

		void normalizePlane(vec4p& plane);

		void extractFrustumPlanes(const mat4& m, vec4p& l, vec4p& r, vec4p& t, vec4p& b, vec4p& n, vec4p& f);
	}
		
}

