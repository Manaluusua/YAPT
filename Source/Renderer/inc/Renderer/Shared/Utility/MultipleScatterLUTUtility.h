#pragma once

#include <Math/RandUtility.h>
#include <Math/GGX.h>
#include <Math/MiscBrdf.h>
#include <Renderer/RendererCommonTypesUtility.h>

#define SS_ALBEDO_LUT_DIM 32
#define SSMS_ALBEDO_LUT_DIM 16
#define SS_ALBEDO_TRANSLUCENT_LUT_DIM 16
#define SS_ALBEDO_SHEEN_LUT_DIM 16
#define MS_LUT_FORMAT (ResourceFormat::R32_SFLOAT)

#define MIN_COS_THETA 0.0001f


namespace YAPT
{
	float sampleLut(float u, const float* lut, uint32_t dimension)
	{
		u = glm::clamp(u, 0.f, 1.f);
		float v = u * dimension;
		v = min(v, float(dimension - 1));

		float a = lut[uint32_t(floor(v))];
		float b = lut[uint32_t(ceil(v))];
		float t = glm::fract(v);
		return (1.f - t) * a + t * b;
	}

	float sampleLut(glm::vec2 uv, const float* lut, const glm::uvec2& lutDimensions)
	{
		auto getFromCoords = [&lut, &lutDimensions](float x, float y)
		{
			return lut[size_t(y * lutDimensions.x + x)];
		};

		uv = glm::clamp(uv, 0.f, 1.f);
		glm::vec2 v = uv * glm::vec2(lutDimensions);
		v = min(v, glm::vec2(lutDimensions - glm::uvec2(1)));

		float a = getFromCoords(floor(v.x), floor(v.y));
		float b = getFromCoords(ceil(v.x), floor(v.y));
		float c = getFromCoords(floor(v.x), ceil(v.y));
		float d = getFromCoords(ceil(v.x), ceil(v.y));

		float tx = glm::fract(v.x);
		float ty = glm::fract(v.y);

		float ab = (1.f - tx) * a + tx * b;
		float cd = (1.f - tx) * c + tx * d;
		return (1.f - ty) * ab + ty * cd;
	}

	float sampleLut(glm::vec3 uv, const float* lut, const glm::uvec3& lutDimensions)
	{
		auto getFromCoords = [&lut, &lutDimensions](float x, float y, float z)
		{
			return lut[size_t(z * (lutDimensions.x * lutDimensions.y) + y * lutDimensions.x + x)];
		};

		uv = glm::clamp(uv, 0.f, 1.f);
		glm::vec3 v = uv * glm::vec3(lutDimensions);
		v = min(v, glm::vec3(lutDimensions - glm::uvec3(1)));

		size_t zCoord = size_t(glm::floor(v.z));
		float a = sampleLut(glm::vec2(uv.x, uv.y), lut + zCoord * lutDimensions.x * lutDimensions.y, glm::vec2(lutDimensions.x, lutDimensions.y));

		zCoord = min(zCoord + 1, size_t(lutDimensions.z - 1));
		float b = sampleLut(glm::vec2(uv.x, uv.y), lut + zCoord * lutDimensions.x * lutDimensions.y, glm::vec2(lutDimensions.x, lutDimensions.y));
		float t = glm::fract(v.z);

		return (1.f - t) * a + t * b;
	}




	glm::vec3 getEnergyCompensationKulla(const glm::vec3& fms, const glm::vec3& singleScatterAlbedo, float dotWo, float dotWi, float linearRoughness, uint32_t lutDimensions, const float* singleScatteringAlbedoLut, const float* singleScatteringAverageAlbedoLut)
	{
		dotWo = MathUtils::saturate(dotWo);
		dotWi = MathUtils::saturate(dotWi);

		float eo = 1.f - sampleLut(glm::vec2(dotWo, linearRoughness), singleScatteringAlbedoLut, glm::vec2(lutDimensions, lutDimensions));
		float ei = 1.f - sampleLut(glm::vec2(dotWi, linearRoughness), singleScatteringAlbedoLut, glm::vec2(lutDimensions, lutDimensions));
		float eAvg = sampleLut(linearRoughness, singleScatteringAverageAlbedoLut, lutDimensions);

		float ems = eo * ei / max(0.00001f, PI - eAvg);
		return fms * ems;

	}

	glm::vec3 getEnergyCompensationTurquin(const glm::vec3& fms, const glm::vec3& singleScatterAlbedo, float dotWo, float dotWi, float linearRoughness, uint32_t lutDimensions, const float* singleScatteringAlbedoLut, const float* singleScatteringAverageAlbedoLut)
	{
		dotWo = MathUtils::saturate(dotWo);
		dotWi = MathUtils::saturate(dotWi);

		float eo = sampleLut(glm::vec2(dotWo, linearRoughness), singleScatteringAlbedoLut, glm::vec2(lutDimensions, lutDimensions));
		glm::vec3 k = fms * (1.f - eo) / eo;
		return  k * singleScatterAlbedo;
	}

	float getAvgFresnel(float etaR)
	{
		float fAvg;
		if (etaR < 1)
		{
			fAvg = 0.997118f + 0.1014f * etaR - 0.965241f * etaR * etaR - 0.130607f * etaR * etaR * etaR;
		}
		else
		{
			fAvg = (etaR - 1.f) / (4.08567f + 1.00071f * etaR);
		}

		return fAvg;
	}

	float getReflectionRatio(float etaR, float linearRoughness, const float* singleScatteringAverageAlbedoDenserLut, const float* singleScatteringAverageAlbedoLighterLut)
	{
		float a = getAvgFresnel(etaR);
		float b = sampleLut(glm::vec2(linearRoughness, 1.f / etaR), etaR < 1.f ? singleScatteringAverageAlbedoDenserLut : singleScatteringAverageAlbedoLighterLut, glm::vec2(SS_ALBEDO_TRANSLUCENT_LUT_DIM, SS_ALBEDO_TRANSLUCENT_LUT_DIM));
		float c = getAvgFresnel(1.f / etaR);
		float d = sampleLut(glm::vec2(linearRoughness, etaR), etaR < 1.f ? singleScatteringAverageAlbedoLighterLut : singleScatteringAverageAlbedoDenserLut, glm::vec2(SS_ALBEDO_TRANSLUCENT_LUT_DIM, SS_ALBEDO_TRANSLUCENT_LUT_DIM));
		float e = etaR * etaR;

		float x = (b - 1.f) * (c - 1.f) * e / SAFE_DIVISOR(a * d - a + b * c * e - b * e - c * e - d + e + 1.f);
		return x;
	}

	float getEnergyCompensationTranslucent(float etaR, float dotWo, float dotWi, float linearRoughness, const glm::vec3& singleScatter, const float* singleScatteringAlbedoDenserLut, const float* singleScatteringAverageAlbedoDenserLut, const float* singleScatteringAlbedoLighterLut, const float* singleScatteringAverageAlbedoLighterLut)
	{
		float dirAlbedoWo;
		float dirAlbedoWi;
		float avgDirAlbedo;

		if (etaR < 1.f)
		{
			dirAlbedoWo = sampleLut(glm::vec3(dotWo, linearRoughness, etaR), singleScatteringAlbedoLighterLut, glm::vec3(SS_ALBEDO_TRANSLUCENT_LUT_DIM, SS_ALBEDO_TRANSLUCENT_LUT_DIM, SS_ALBEDO_TRANSLUCENT_LUT_DIM));
		}
		else
		{
			dirAlbedoWo = sampleLut(glm::vec3(dotWo, linearRoughness, etaR), singleScatteringAlbedoDenserLut, glm::vec3(SS_ALBEDO_TRANSLUCENT_LUT_DIM, SS_ALBEDO_TRANSLUCENT_LUT_DIM, SS_ALBEDO_TRANSLUCENT_LUT_DIM));
		}

		float eta2 = dotWi < 0.f ? 1.f / etaR : etaR;

		if (eta2 < 1.f)
		{
			dirAlbedoWi = sampleLut(glm::vec3(abs(dotWi), linearRoughness, eta2), singleScatteringAlbedoLighterLut, glm::vec3(SS_ALBEDO_TRANSLUCENT_LUT_DIM, SS_ALBEDO_TRANSLUCENT_LUT_DIM, SS_ALBEDO_TRANSLUCENT_LUT_DIM));
			avgDirAlbedo = sampleLut(glm::vec2(linearRoughness, eta2), singleScatteringAverageAlbedoLighterLut, glm::vec2(SS_ALBEDO_TRANSLUCENT_LUT_DIM, SS_ALBEDO_TRANSLUCENT_LUT_DIM));
		}
		else
		{
			dirAlbedoWi = sampleLut(glm::vec3(abs(dotWi), linearRoughness, eta2), singleScatteringAlbedoDenserLut, glm::vec3(SS_ALBEDO_TRANSLUCENT_LUT_DIM, SS_ALBEDO_TRANSLUCENT_LUT_DIM, SS_ALBEDO_TRANSLUCENT_LUT_DIM));
			avgDirAlbedo = sampleLut(glm::vec2(linearRoughness, eta2), singleScatteringAverageAlbedoDenserLut, glm::vec2(SS_ALBEDO_TRANSLUCENT_LUT_DIM, SS_ALBEDO_TRANSLUCENT_LUT_DIM));

		}

		float ems = ((1.f - dirAlbedoWo) * (1.f - dirAlbedoWi)) / max(0.00001f, PI - avgDirAlbedo);
		float ratio = getReflectionRatio(etaR, linearRoughness, singleScatteringAverageAlbedoDenserLut, singleScatteringAverageAlbedoLighterLut);


		if (dotWi < 0.f)
		{
			ratio = 1.f - ratio;
		}

		ems *= ratio;

		return ems;
	}


	float integrateDirectionalSingleScatterGGXAlbedoNoFresnel(float roughness, float cosTheta)
	{
		constexpr uint32_t numberOfSamples = 256;
		float accum = 0;
		float a = roughness * roughness;

		float sinTheta = sqrt(1.f - MathUtils::sqr(cosTheta));
		glm::vec3 wo = glm::vec3(sinTheta, cosTheta, 0);

		for (uint32_t i = 0; i < numberOfSamples; ++i)
		{
			vec2 sample = MathUtils::hammersley(i, numberOfSamples);
			glm::vec3 wm = sampleWM(wo, a, a, sample.x, sample.y);
			glm::vec3 wi = glm::reflect(-wo, wm);


			//only accept samples from upper hemisphere
			if (wi.y <= 0.0f || dot(wi, wm) <= 0.f || dot(wo, wm) <= 0.f)
			{
				continue;
			}

			float G2 = G2GGX(wo, wi, wm, a, a);
			float D = DGGX(wm, a, a);

			float pdf = pdfWM(wo, wm, a, a) * jReflection(wo, wm);

			float weight = G2 * D / max(4.f * wo.y * wi.y, 0.000001f);

			if (pdf > 0.f)
			{
				accum += weight * abs(wi.y) / pdf;
			}
		}

		return accum / numberOfSamples;
	}


	float integrateDirectionalSingleScatterGGXAlbedo(float etaR, float roughness, float cosTheta)
	{
		constexpr uint32_t numberOfSamples = 256;
		float accum = 0;
		float a = roughness * roughness;

		float sinTheta = sqrt(1.f - MathUtils::sqr(cosTheta));
		glm::vec3 wo = glm::vec3(sinTheta, cosTheta, 0);

		for (uint32_t i = 0; i < numberOfSamples; ++i)
		{
			vec2 sample = MathUtils::hammersley(i, numberOfSamples);
			glm::vec3 wm = sampleWM(wo, a, a, sample.x, sample.y);
			glm::vec3 wi = glm::reflect(-wo, wm);


			//only accept samples from upper hemisphere
			if (wi.y <= 0.0f || dot(wi, wm) <= 0.f || dot(wo, wm) <= 0.f)
			{
				continue;
			}

			float G2 = G2GGX(wo, wi, wm, a, a);
			float F = fresnelDielectricDielectric2(etaR, dot(wo, wm));
			float D = DGGX(wm, a, a);

			float pdf = pdfWM(wo, wm, a, a) * jReflection(wo, wm);

			float weight = F * G2 * D / max(4.f * wo.y * wi.y, 0.000001f);

			if (pdf > 0.f)
			{
				accum += weight * abs(wi.y) / pdf;
			}
		}

		return accum / numberOfSamples;
	}

	float integrateDirectionalSingleScatterGGXAlbedoTranslucent(float etaR, float roughness, float cosTheta)
	{
		constexpr uint32_t numberOfSamples = 300;
		glm::vec3 samples[numberOfSamples];
		MathUtils::generateHaltonSequence(numberOfSamples, samples, 0);
		float accum = 0;

		float a = roughness * roughness;

		float sinTheta = sqrt(1.f - MathUtils::sqr(cosTheta));
		glm::vec3 wo = glm::vec3(sinTheta, cosTheta, 0);

		for (uint32_t i = 0; i < numberOfSamples; ++i)
		{
			glm::vec3 sample = samples[i];
			glm::vec3 wi;
			sampleGGXDielectricTranslucent(etaR, a, a, wo, sample, wi);
			if (dot(wi, wi) == 0.f) continue; //failed to sample

			float pdf = pdfGGXDielectricTranslucent(etaR, wo, wi, a, a);
			float weight = evaluateGGXDielectricTranslucent(etaR, a, a, wo, wi).x;

			if (pdf > 0.f)
			{
				accum += weight * abs(wi.y) / pdf;
			}


		}

		return accum / numberOfSamples;
	}

	float integrateSingleAndMultiScatterGGXAlbedo(float roughness, float cosTheta, float etaR, const float* singleScatteringAlbedoLut, const float* singleScatteringAverageAlbedoLut)
	{
		constexpr uint32_t numberOfSamples = 256;
		float accum = 0;
		float a = roughness * roughness;

		float sinTheta = sqrt(1.f - MathUtils::sqr(cosTheta));
		glm::vec3 wo = glm::vec3(sinTheta, cosTheta, 0);

		for (uint32_t i = 0; i < numberOfSamples; ++i)
		{
			vec2 sample = MathUtils::hammersley(i, numberOfSamples);
			glm::vec3 wm = sampleWM(wo, a, a, sample.x, sample.y);
			glm::vec3 wi = glm::reflect(-wo, wm);


			//only accept samples from upper hemisphere
			if (wi.y < 0.0f || dot(wi, wm) < 0.f || dot(wo, wm) < 0.f)
			{
				continue;
			}

			float G2 = G2GGX(wo, wi, wm, a, a);
			float F = fresnelDielectricDielectric2(etaR, dot(wo, wm));
			float D = DGGX(wm, a, a);

			float pdf = pdfWM(wo, wm, a, a) * jReflection(wo, wm);

			float weight = F * G2 * D / max(4.f * wo.y * wi.y, 0.000001f);

			glm::vec3 fms(F);
			glm::vec3 ss(weight);

			weight += getEnergyCompensationKulla(fms, ss, cosTheta, wi.y, roughness, SS_ALBEDO_LUT_DIM, singleScatteringAlbedoLut, singleScatteringAverageAlbedoLut).x;
			if (pdf > 0.f)
			{
				accum += weight * abs(wi.y) / pdf;
			}


		}

		return  accum / numberOfSamples;
	}


	float integrateDirectionalMultiScatterGGXAlbedoTranslucent(float etaR, float roughness, float cosTheta, const float* singleScatteringAlbedoDenserLut, const float* singleScatteringAverageAlbedoDenserLut, const float* singleScatteringAlbedoLighterLut, const float* singleScatteringAverageAlbedoLighterLut)
	{
		constexpr uint32_t numberOfSamples = 300;
		glm::vec3 samples[numberOfSamples];
		MathUtils::generateHaltonSequence(numberOfSamples, samples, 0);
		float accum = 0;

		float a = roughness * roughness;

		float sinTheta = sqrt(1.f - MathUtils::sqr(cosTheta));
		glm::vec3 wo = glm::vec3(sinTheta, cosTheta, 0);

		for (uint32_t i = 0; i < numberOfSamples; ++i)
		{
			glm::vec3 sample = samples[i];
			glm::vec3 wi;
			sampleGGXDielectricTranslucent(etaR, a, a, wo, sample, wi);
			if (dot(wi, wi) == 0.f) continue; //failed to sample

			float pdf = pdfGGXDielectricTranslucent(etaR, wo, wi, a, a);
			float weight = evaluateGGXDielectricTranslucent(etaR, a, a, wo, wi).x;
			if (pdf > 0.f)
			{
				float energyComp = getEnergyCompensationTranslucent(etaR, wo.y, wi.y, roughness, glm::vec3(weight),
					singleScatteringAlbedoDenserLut, singleScatteringAverageAlbedoDenserLut, singleScatteringAlbedoLighterLut, singleScatteringAverageAlbedoLighterLut);
				accum += energyComp * abs(wi.y) / pdf;
			}


		}

		return accum / numberOfSamples;
	}

	float integrateDirectionalSingleScatterAlbedoSheenNoFresnel(float roughness, float cosTheta)
	{
		constexpr uint32_t numberOfSamples = 256;
		float accum = 0;
		float a = max(roughness, 0.07f);

		float sinTheta = sqrt(1.f - MathUtils::sqr(cosTheta));
		glm::vec3 wo = glm::vec3(sinTheta, cosTheta, 0);

		for (uint32_t i = 0; i < numberOfSamples; ++i)
		{
			vec2 sample = MathUtils::hammersley(i, numberOfSamples);
			glm::vec3 wi = sampleHemisphere(sample);
			glm::vec3 wm = glm::normalize(wi + wo);

			//only accept samples from upper hemisphere
			if (wi.y <= 0.0f)
			{
				continue;
			}

			float G = GSheen(wo, wi, a);
			float D = DSheen(wm, a);

			float pdf = pdfHemisphere(wi);

			float weight = G * D / max(4.f * wo.y * wi.y, 0.000001f);

			if (pdf > 0.f)
			{
				accum += weight * abs(wi.y) / pdf;
			}
		}

		return accum / numberOfSamples;
	}
}