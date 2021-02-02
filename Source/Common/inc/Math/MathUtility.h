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

		 
		inline glm::vec3 sphericalToCartesian(float phi, float theta)
		{
			float cosTheta = glm::cos(theta);
			float sinTheta = sqrt(1.f - sqr(cosTheta));
			return glm::vec3(cos(phi) * sinTheta, cosTheta, sin(phi) * sinTheta);
		}

		void calculateArbitraryOrthogonalVector(const glm::vec3& vec, glm::vec3& orthogonalOut);

		void calculateTangentAndBitangentFromPositionAndUv(const glm::vec3& p1, const glm::vec3& p2, const glm::vec3& p3,
			const glm::vec2& t1, const glm::vec2& t2, const glm::vec2& t3, glm::vec3& outTangent, glm::vec3& outBitangent);

		void orthogonalizeAndNormalizeTangent(const glm::vec3& tangent, const glm::vec3& normal, glm::vec3& tangentOut);

		float calculateHandedness(const glm::vec3& tangent, const glm::vec3& bitangent, const glm::vec3& normal);

		void normalizePlane(glm::vec4& plane);

		void extractFrustumPlanes(const mat4& m, glm::vec4& l, glm::vec4& r, glm::vec4& t, glm::vec4& b, glm::vec4& n, glm::vec4& f);
	}
		
}

