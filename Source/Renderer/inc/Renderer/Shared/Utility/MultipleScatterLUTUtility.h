#pragma once

#include <Math/RandUtility.h>
#include <Math/GGX.h>
#include <Math/MiscBrdf.h>
#include <Gfx/GfxBasicTypesUtility.h>

#define SS_ALBEDO_LUT_DIM 32
#define SSMS_ALBEDO_LUT_DIM 16
#define SS_ALBEDO_TRANSLUCENT_LUT_DIM 16
#define SS_ALBEDO_SHEEN_LUT_DIM 16
#define MS_LUT_FORMAT (ResourceFormat::R32_SFLOAT)

#define MIN_ROUGHNESS 0.01f
#define MIN_COS_THETA 0.001f
#define ETA_EPSILON_MIN_FROM_1 0.01f


namespace YAPT
{
	float ensureValidEtaR(float etaR)
	{
		//don't allow too close to 1
		if (etaR > 1)
		{
			return max(etaR, 1.f + ETA_EPSILON_MIN_FROM_1);
		}
		else
		{
			return min(etaR, 1.f - ETA_EPSILON_MIN_FROM_1);
		}
	}

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

	float sampleLut(vec2p uv, const float* lut, const glm::uvec2& lutDimensions)
	{
		auto getFromCoords = [&lut, &lutDimensions](float x, float y)
		{
			return lut[size_t(y * lutDimensions.x + x)];
		};

		uv = glm::clamp(uv, 0.f, 1.f);
		vec2p v = uv * vec2p(lutDimensions);
		v = min(v, vec2p(lutDimensions - glm::uvec2(1)));

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

	float sampleLut(vec3p uv, const float* lut, const glm::uvec3& lutDimensions)
	{
		auto getFromCoords = [&lut, &lutDimensions](float x, float y, float z)
		{
			return lut[size_t(z * (lutDimensions.x * lutDimensions.y) + y * lutDimensions.x + x)];
		};

		uv = glm::clamp(uv, 0.f, 1.f);
		vec3p v = uv * vec3p(lutDimensions);
		v = min(v, vec3p(lutDimensions - glm::uvec3(1)));

		size_t zCoord = size_t(glm::floor(v.z));
		float a = sampleLut(vec2p(uv.x, uv.y), lut + zCoord * lutDimensions.x * lutDimensions.y, vec2p(lutDimensions.x, lutDimensions.y));

		zCoord = min(zCoord + 1, size_t(lutDimensions.z - 1));
		float b = sampleLut(vec2p(uv.x, uv.y), lut + zCoord * lutDimensions.x * lutDimensions.y, vec2p(lutDimensions.x, lutDimensions.y));
		float t = glm::fract(v.z);

		return (1.f - t) * a + t * b;
	}




	vec3p getEnergyCompensationKulla(const vec3p& fms, const vec3p& singleScatterAlbedo, float dotWo, float dotWi, float linearRoughness, uint32_t lutDimensions, const float* singleScatteringAlbedoLut, const float* singleScatteringAverageAlbedoLut)
	{
		dotWo = MathUtils::saturate(dotWo);
		dotWi = MathUtils::saturate(dotWi);

		float eo = 1.f - sampleLut(vec2p(dotWo, linearRoughness), singleScatteringAlbedoLut, vec2p(lutDimensions, lutDimensions));
		float ei = 1.f - sampleLut(vec2p(dotWi, linearRoughness), singleScatteringAlbedoLut, vec2p(lutDimensions, lutDimensions));
		float eAvg = sampleLut(linearRoughness, singleScatteringAverageAlbedoLut, lutDimensions);

		float ems = eo * ei / max(0.0001f, (1.f - eAvg) * PI);
		return fms * ems;

	}

	vec3p getEnergyCompensationTurquin(const vec3p& fms, const vec3p& singleScatterAlbedo, float dotWo, float dotWi, float linearRoughness, uint32_t lutDimensions, const float* singleScatteringAlbedoLut, const float* singleScatteringAverageAlbedoLut)
	{
		dotWo = MathUtils::saturate(dotWo);
		dotWi = MathUtils::saturate(dotWi);

		float eo = sampleLut(vec2p(dotWo, linearRoughness), singleScatteringAlbedoLut, vec2p(lutDimensions, lutDimensions));
		vec3p k = fms * (1.f - eo) / eo;
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
		float b = sampleLut(vec2p(linearRoughness, 1.f / etaR), etaR < 1.f ? singleScatteringAverageAlbedoDenserLut : singleScatteringAverageAlbedoLighterLut, vec2p(SS_ALBEDO_TRANSLUCENT_LUT_DIM, SS_ALBEDO_TRANSLUCENT_LUT_DIM));
		float c = getAvgFresnel(1.f / etaR);
		float d = sampleLut(vec2p(linearRoughness, etaR), etaR < 1.f ? singleScatteringAverageAlbedoLighterLut : singleScatteringAverageAlbedoDenserLut, vec2p(SS_ALBEDO_TRANSLUCENT_LUT_DIM, SS_ALBEDO_TRANSLUCENT_LUT_DIM));
		float e = etaR * etaR;

		float x = (b - 1.f) * (c - 1.f) * e / SAFE_DIVISOR(a * d - a + b * c * e - b * e - c * e - d + e + 1.f);
		return x;
	}

	float getEnergyCompensationTranslucent(float etaR, float dotWo, float dotWi, float linearRoughness, const vec3p& singleScatter, const float* singleScatteringAlbedoDenserLut, const float* singleScatteringAverageAlbedoDenserLut, const float* singleScatteringAlbedoLighterLut, const float* singleScatteringAverageAlbedoLighterLut)
	{
		float dirAlbedoWo;
		float dirAlbedoWi;
		float avgDirAlbedo;

		if (etaR < 1.f)
		{
			dirAlbedoWo = sampleLut(vec3p(dotWo, linearRoughness, etaR), singleScatteringAlbedoLighterLut, vec3p(SS_ALBEDO_TRANSLUCENT_LUT_DIM, SS_ALBEDO_TRANSLUCENT_LUT_DIM, SS_ALBEDO_TRANSLUCENT_LUT_DIM));
		}
		else
		{
			dirAlbedoWo = sampleLut(vec3p(dotWo, linearRoughness, etaR), singleScatteringAlbedoDenserLut, vec3p(SS_ALBEDO_TRANSLUCENT_LUT_DIM, SS_ALBEDO_TRANSLUCENT_LUT_DIM, SS_ALBEDO_TRANSLUCENT_LUT_DIM));
		}

		float eta2 = dotWi < 0.f ? 1.f / etaR : etaR;

		if (eta2 < 1.f)
		{
			dirAlbedoWi = sampleLut(vec3p(abs(dotWi), linearRoughness, eta2), singleScatteringAlbedoLighterLut, vec3p(SS_ALBEDO_TRANSLUCENT_LUT_DIM, SS_ALBEDO_TRANSLUCENT_LUT_DIM, SS_ALBEDO_TRANSLUCENT_LUT_DIM));
			avgDirAlbedo = sampleLut(vec2p(linearRoughness, eta2), singleScatteringAverageAlbedoLighterLut, vec2p(SS_ALBEDO_TRANSLUCENT_LUT_DIM, SS_ALBEDO_TRANSLUCENT_LUT_DIM));
		}
		else
		{
			dirAlbedoWi = sampleLut(vec3p(abs(dotWi), linearRoughness, eta2), singleScatteringAlbedoDenserLut, vec3p(SS_ALBEDO_TRANSLUCENT_LUT_DIM, SS_ALBEDO_TRANSLUCENT_LUT_DIM, SS_ALBEDO_TRANSLUCENT_LUT_DIM));
			avgDirAlbedo = sampleLut(vec2p(linearRoughness, eta2), singleScatteringAverageAlbedoDenserLut, vec2p(SS_ALBEDO_TRANSLUCENT_LUT_DIM, SS_ALBEDO_TRANSLUCENT_LUT_DIM));

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
		roughness = max(MIN_ROUGHNESS, roughness);
		float a = roughness * roughness;

		float sinTheta = sqrt(1.f - MathUtils::sqr(cosTheta));
		vec3p wo = vec3p(sinTheta, cosTheta, 0);

		for (uint32_t i = 0; i < numberOfSamples; ++i)
		{
			vec2 sample = MathUtils::hammersley(i, numberOfSamples);
			vec3p wm = sampleWM(wo, a, a, sample.x, sample.y);
			vec3p wi = glm::reflect(-wo, wm);


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
		roughness = max(MIN_ROUGHNESS, roughness);
		float a = roughness * roughness;

		etaR = ensureValidEtaR(etaR);

		float sinTheta = sqrt(1.f - MathUtils::sqr(cosTheta));
		vec3p wo = vec3p(sinTheta, cosTheta, 0);

		for (uint32_t i = 0; i < numberOfSamples; ++i)
		{
			vec2 sample = MathUtils::hammersley(i, numberOfSamples);
			vec3p wm = sampleWM(wo, a, a, sample.x, sample.y);
			vec3p wi = glm::reflect(-wo, wm);


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
		constexpr uint32_t numberOfSamples = 1024;
		vec3p samples[numberOfSamples];
		MathUtils::generateHaltonSequence(numberOfSamples, samples, 0);
		float accum = 0;

		etaR = ensureValidEtaR(etaR);
		roughness = max(MIN_ROUGHNESS, roughness);

		float a = roughness * roughness;

		float sinTheta = sqrt(1.f - MathUtils::sqr(cosTheta));
		vec3p wo = vec3p(sinTheta, cosTheta, 0);

		for (uint32_t i = 0; i < numberOfSamples; ++i)
		{
			vec3p sample = samples[i];
			vec3p wi;
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
		roughness = max(MIN_ROUGHNESS, roughness);
		float a = roughness * roughness;

		etaR = ensureValidEtaR(etaR);

		float sinTheta = sqrt(1.f - MathUtils::sqr(cosTheta));
		vec3p wo = vec3p(sinTheta, cosTheta, 0);

		for (uint32_t i = 0; i < numberOfSamples; ++i)
		{
			vec2 sample = MathUtils::hammersley(i, numberOfSamples);
			vec3p wm = sampleWM(wo, a, a, sample.x, sample.y);
			vec3p wi = glm::reflect(-wo, wm);


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

			vec3p fms(F);
			vec3p ss(weight);

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
		vec3p samples[numberOfSamples];
		MathUtils::generateHaltonSequence(numberOfSamples, samples, 0);
		float accum = 0;
		etaR = ensureValidEtaR(etaR);
		roughness = max(MIN_ROUGHNESS, roughness);
		float a = roughness * roughness;

		float sinTheta = sqrt(1.f - MathUtils::sqr(cosTheta));
		vec3p wo = vec3p(sinTheta, cosTheta, 0);

		for (uint32_t i = 0; i < numberOfSamples; ++i)
		{
			vec3p sample = samples[i];
			vec3p wi;
			sampleGGXDielectricTranslucent(etaR, a, a, wo, sample, wi);
			if (dot(wi, wi) == 0.f) continue; //failed to sample

			float pdf = pdfGGXDielectricTranslucent(etaR, wo, wi, a, a);
			float weight = evaluateGGXDielectricTranslucent(etaR, a, a, wo, wi).x;
			if (pdf > 0.f)
			{
				float energyComp = getEnergyCompensationTranslucent(etaR, wo.y, wi.y, roughness, vec3p(weight),
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
		roughness = max(MIN_ROUGHNESS, roughness);
		float a = max(roughness, 0.07f);

		float sinTheta = sqrt(1.f - MathUtils::sqr(cosTheta));
		vec3p wo = vec3p(sinTheta, cosTheta, 0);

		for (uint32_t i = 0; i < numberOfSamples; ++i)
		{
			vec2 sample = MathUtils::hammersley(i, numberOfSamples);
			vec3p wi = sampleHemisphere(sample);
			vec3p wm = glm::normalize(wi + wo);

			//only accept samples from upper hemisphere
			if (wi.y <= 0.0f)
			{
				continue;
			}

			float G = GSheen(wo, wi, a);
			float D = DSheen(wm, a);

			float pdf = pdfHemisphere();

			float weight = G * D / max(4.f * wo.y * wi.y, 0.000001f);

			if (pdf > 0.f)
			{
				accum += weight * abs(wi.y) / pdf;
			}
		}

		return accum / numberOfSamples;
	}
}