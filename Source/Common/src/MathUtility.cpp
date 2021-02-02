#include <Math/MathUtility.h>

namespace YAPT
{
	namespace MathUtils
	{
		void calculateArbitraryOrthogonalVector(const glm::vec3& vec, glm::vec3& orthogonalOut)
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

		void calculateTangentAndBitangentFromPositionAndUv(const glm::vec3& p1, const glm::vec3& p2, const glm::vec3& p3,
			const glm::vec2& t1, const glm::vec2& t2, const glm::vec2& t3, glm::vec3& outTangent, glm::vec3& outBitangent)
		{
			glm::vec3 dp1 = p2 - p1;
			glm::vec3 dp2 = p3 - p1;

			glm::vec2 dt1 = t2 - t1;
			glm::vec2 dt2 = t3 - t1;

			float denom = dt1[0] * dt2[1] - dt2[0] * dt1[1];
			float r = denom == 0.f ? 0.f : 1.f / denom;

			outTangent = glm::vec3((dt2[1] * dp1[0] - dt1[1] * dp2[0]) * r, (dt2[1] * dp1[1] - dt1[1] * dp2[1]) * r,
				(dt2[1] * dp1[2] - dt1[1] * dp2[2]) * r);
			outBitangent = glm::vec3((dt1[0] * dp2[0] - dt2[0] * dp1[0]) * r, (dt1[0] * dp2[1] - dt2[0] * dp1[1]) * r,
				(dt1[0] * dp2[2] - dt2[0] * dp1[2]) * r);
		}

		void orthogonalizeAndNormalizeTangent(const glm::vec3& tangent, const glm::vec3& normal, glm::vec3& tangentOut)
		{
			float d = dot(normal, tangent);
			glm::vec3 temp = tangent - (normal * d);
			tangentOut = glm::normalize(temp);
		}

		float calculateHandedness(const glm::vec3& tangent, const glm::vec3& bitangent, const glm::vec3& normal)
		{
			glm::vec3 crossProd;
			crossProd = glm::cross(normal, tangent);
			return dot(crossProd, bitangent) < 0 ? -1.f : 1.f;
		}
		void normalizePlane(glm::vec4& plane)
		{
			glm::vec3 normal(plane);
			const float d = glm::length(normal);
			if (d == 0.f)
				return;
			plane /= d;
		}

		void extractFrustumPlanes(const mat4& m, glm::vec4& l, glm::vec4& r, glm::vec4& t, glm::vec4& b, glm::vec4& n, glm::vec4& f)
		{
			glm::vec4 x(m[0][0], m[1][0], m[2][0], m[3][0]);
			glm::vec4 y(m[0][1], m[1][1], m[2][1], m[3][1]);
			glm::vec4 z(m[0][2], m[1][2], m[2][2], m[3][2]);
			glm::vec4 w(m[0][3], m[1][3], m[2][3], m[3][3]);
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