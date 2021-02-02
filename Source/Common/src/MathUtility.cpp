#include <Math/MathUtility.h>

namespace YAPT
{
	namespace MathUtils
	{
		void calculateArbitraryOrthogonalVector(const vec3p& vec, vec3p& orthogonalOut)
		{
			float smallest = std::abs(vec[0]);
			int smallestInd = 0;

			if (std::abs(vec[1]) < smallest)
			{
				smallest = std::abs(vec[1]);
				smallestInd = 1;
			}

			if (std::abs(vec[2]) < smallest)
			{
				smallest = std::abs(vec[2]);
				smallestInd = 2;
			}

			if (smallestInd == 0)
			{
				orthogonalOut[0] = 0;
				orthogonalOut[1] = -vec[2];
				orthogonalOut[2] = vec[1];
			}
			else if (smallestInd == 1)
			{
				orthogonalOut[0] = -vec[2];
				orthogonalOut[1] = 0;
				orthogonalOut[2] = vec[0];
			}
			else if (smallestInd == 2)
			{
				orthogonalOut[0] = -vec[1];
				orthogonalOut[1] = vec[0];
				orthogonalOut[2] = 0;
			}
		}

		void calculateTangentAndBitangentFromPositionAndUv(const vec3p& p1, const vec3p& p2, const vec3p& p3,
			const vec2p& t1, const vec2p& t2, const vec2p& t3, vec3p& outTangent, vec3p& outBitangent)
		{
			vec3p dp1 = p2 - p1;
			vec3p dp2 = p3 - p1;

			vec2p dt1 = t2 - t1;
			vec2p dt2 = t3 - t1;

			float denom = dt1[0] * dt2[1] - dt2[0] * dt1[1];
			float r = denom == 0.f ? 0.f : 1.f / denom;

			outTangent = vec3p((dt2[1] * dp1[0] - dt1[1] * dp2[0]) * r, (dt2[1] * dp1[1] - dt1[1] * dp2[1]) * r,
				(dt2[1] * dp1[2] - dt1[1] * dp2[2]) * r);
			outBitangent = vec3p((dt1[0] * dp2[0] - dt2[0] * dp1[0]) * r, (dt1[0] * dp2[1] - dt2[0] * dp1[1]) * r,
				(dt1[0] * dp2[2] - dt2[0] * dp1[2]) * r);
		}

		void orthogonalizeAndNormalizeTangent(const vec3p& tangent, const vec3p& normal, vec3p& tangentOut)
		{
			float d = dot(normal, tangent);
			vec3p temp = tangent - (normal * d);
			tangentOut = glm::normalize(temp);
		}

		float calculateHandedness(const vec3p& tangent, const vec3p& bitangent, const vec3p& normal)
		{
			vec3p crossProd;
			crossProd = glm::cross(normal, tangent);
			return dot(crossProd, bitangent) < 0 ? -1.f : 1.f;
		}
		void normalizePlane(vec4p& plane)
		{
			vec3p normal(plane);
			const float d = glm::length(normal);
			if (d == 0.f)
				return;
			plane /= d;
		}

		void extractFrustumPlanes(const mat4& m, vec4p& l, vec4p& r, vec4p& t, vec4p& b, vec4p& n, vec4p& f)
		{
			vec4p x(m[0][0], m[1][0], m[2][0], m[3][0]);
			vec4p y(m[0][1], m[1][1], m[2][1], m[3][1]);
			vec4p z(m[0][2], m[1][2], m[2][2], m[3][2]);
			vec4p w(m[0][3], m[1][3], m[2][3], m[3][3]);
			l = w + x;
			r = w - x;
			t = w - y;
			b = w + y;
			n = w + z;
			f = w - z;

			normalizePlane(l);
			normalizePlane(r);
			normalizePlane(t);
			normalizePlane(b);
			normalizePlane(n);
			normalizePlane(f);


		}

	}
}