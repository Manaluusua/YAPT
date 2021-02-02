#pragma once
#include <Common/CommonUtilities.h>
#include <Math/Math.h>
#include <Math/Fresnel.h>
#include <Math/MathUtility.h>


namespace YAPT
{

	constexpr float lerp(float x, float y, float t)
	{
		return x * (1.f - t) + y * t;

	}

	glm::vec3 sampleHemisphere(const glm::vec2& sample)
	{
		float phi = 2.0f * PI * sample.y;
		float cosTheta = sqrt(1.f - sample.x);
		float sinTheta = sqrt(1.f - MathUtils::sqr(cosTheta));
		glm::vec3 wi = glm::vec3(cos(phi) * sinTheta, cosTheta, sin(phi) * sinTheta);
		return wi;
	}

	glm::vec3 evaluateLambertian(const glm::vec3& albedo, const glm::vec3& wi)
	{
		return albedo / glm::vec3(PI);
	}

	float pdfHemisphere(const glm::vec3& wi) {
		return MathUtils::saturate(wi.y) / PI;
	}

	//from imageworks sheen: http://www.aconty.com/pdf/s2017_pbs_imageworks_sheen.pdf
	float GLSheen(float x, float a, float b, float c, float d, float e)
	{
		return (a / (1 + b * pow(x, c))) + d * x + e;
	}

	float GLambdaSheen(float roughness, float cosTheta)
	{
		float a0 = 25.3245f;
		float b0 = 3.32435f;
		float c0 = 0.16801f;
		float d0 = -1.27393f;
		float e0 = -4.85967f;

		float a1 = 21.5473f;
		float b1 = 3.82987f;
		float c1 = 0.19823f;
		float d1 = -1.97760f;
		float e1 = -4.32054f;

		float t = 1.f - roughness;
		t *= t;

		float a = lerp(a1, a0, t);
		float b = lerp(b1, b0, t);
		float c = lerp(c1, c0, t);
		float d = lerp(d1, d0, t);
		float e = lerp(e1, e0, t);

		float exponent;
		if (cosTheta < 0.5f)
		{
			exponent = GLSheen(cosTheta, a, b, c, d, e);
		}
		else
		{
			exponent = 2.f * GLSheen(0.5f, a, b, c, d, e) - GLSheen(1.f - cosTheta, a, b, c, d, e);
		}
		return exp(exponent);
	}

	float DSheen(const glm::vec3& wm, float r)
	{
		float rInv = 1.f / r;
		float sinTheta2 = 1.f - wm.y * wm.y;
		float v = (2.f + rInv) * pow(sinTheta2, rInv * 0.5f);
		return v / (2.f * PI);
	}

	float GSheen(const glm::vec3& wo, const glm::vec3& wi, float r)
	{

		float lambdaI = GLambdaSheen(r, wi.y); //apply the "light side softening"? breaks reciprocity
		float lambdaO = GLambdaSheen(r, wo.y);

		return 1.f / (1.f + lambdaI + lambdaO);
	}
}