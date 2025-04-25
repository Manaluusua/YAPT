#ifndef MATERIAL_LAYERS_HLSL_INCL
#define MATERIAL_LAYERS_HLSL_INCL

#include "../common/bsdfSample.hlsl"
#include "raytraceCommonResources.hlsl"
#include "hitshadersCommon.hlsl"
#include "../common/multiscatter.hlsl"

struct MaterialLayer
{
	float3 sampleWi(float3 wo, float rand);
	SpectralSamples evaluate(float3 wo, float3 wi);
	float pdf(float3 wo, float3 wi);
	float getEnergyLeftAfterLayer(float3 wo, float3 wi, float layerWeight);
};

struct ReflectionDielectric: MaterialLayer
{
	static ReflectionDielectric init(float2 a2, float linearRoughness, float eta)
	{
		ReflectionDielectric l;
		l.roughness2 = a2;
		l.etaR = eta;
		l.linearRoughness = linearRoughness;
		return l;
	}

	float3 sampleWi(float3 wo, float rand)
	{
		return sampleGGXReflectionDielectric(roughness2.x, roughness2.y, wo, rand);
	}

	SpectralSamples evaluate(float3 wo, float3 wi)
	{
		SpectralSamples w;
		

		if (onSameHemisphere(wo, wi))
		{
			float weight = evaluateGGXReflectionDielectric(etaR, roughness2.x, roughness2.y, wo, wi);
			
			//multiscatter
			float3 wm = normalize(wo + wi);
			float fms = getFmsDielectric(etaR, abs(dot(wo, wm)));
			float msbrdf = getEnergyCompensation(fms, abs(wo.y), abs(wi.y), linearRoughness, weight);
			weight += msbrdf;
			weight *= abs(wi.y);

			w.set(weight);
		}
		else
		{
			w.set(0);
		}

		return w;
	}

	float pdf(float3 wo, float3 wi)
	{
		if (!onSameHemisphere(wo, wi)) return 0.f;
		return pdfGGXReflectionDielectric(wo, wi, roughness2.x, roughness2.y);
	}

	float getEnergyLeftAfterLayer(float3 wo, float3 wi, float layerWeight)
	{
		return getEnergyRemainingAfterSpecular(etaR, wo.y, wi.y, linearRoughness, layerWeight);
	}

	float2 roughness2;
	float etaR;
	float linearRoughness;
};


struct ReflectionSheen : MaterialLayer
{
	static ReflectionSheen init(SpectralSamples sheenTint, float r)
	{
		ReflectionSheen l;
		l.roughness = r;
		l.color = sheenTint;
		return l;
	}

	float3 sampleWi(float3 wo, float rand)
	{
		return sampleSheen(roughness, wo, rand);
	}

	SpectralSamples evaluate(float3 wo, float3 wi)
	{
		SpectralSamples w;

		if (onSameHemisphere(wo, wi))
		{
			w = evaluateSheen(color, roughness, wo, wi);
			w = w * abs(wi.y);

		}
		else
		{
			w.set(0);
		}

		return w;
	}

	float pdf(float3 wo, float3 wi)
	{
		if (!onSameHemisphere(wo, wi)) return 0.f;
		return pdfSheen(wo, wi, roughness);
	}

	float getEnergyLeftAfterLayer(float3 wo, float3 wi, float layerWeight)
	{
		return getEnergyRemainingAfterSheen(wo.y, wi.y, roughness, layerWeight);
	}

	float roughness;
	SpectralSamples color;
};

struct ReflectionConductor : MaterialLayer
{
	static ReflectionConductor init(SpectralSamples colorIncident, SpectralSamples colorGlancing, float2 a2, float linearRoughness, float fromIOR)
	{
		ReflectionConductor l;
		l.roughness2 = a2;
		l.linearRoughness = linearRoughness;
		l.color0 = colorIncident;
		l.color1 = colorGlancing;
		l.fromIOR = fromIOR;
		return l;
	}

	float3 sampleWi(float3 wo, float rand)
	{
		return sampleGGXReflectionConductor(roughness2.x, roughness2.y, wo, rand);
	}

	SpectralSamples evaluate(float3 wo, float3 wi)
	{
		SpectralSamples w;

		if (onSameHemisphere(wo, wi))
		{
			SpectralSamples real;
			SpectralSamples img;

			for (uint i = 0; i < color0.getSampleCount(); ++i)
			{
				float2 v = getConductorRefractiveIndexAndExtinctionthroughputSquared(color0[i], color1[i]);
				real.setInd(i, v.x);
				img.setInd(i, sqrt(v.y));
			}

			SpectralSamples etaR = real / fromIOR;
			SpectralSamples etaK = img / fromIOR;

			//single scatter
			w = evaluateGGXReflectionConductor(etaR, etaK, roughness2.x, roughness2.y, wo, wi);

			//multiscatter
			float3 wm = normalize(wo + wi);
			SpectralSamples fms = getFmsConductor(etaR, etaK, dot(wo, wm));
			SpectralSamples msbrdf = getEnergyCompensation(fms, wo.y, wi.y, linearRoughness, w);
			w = (w + msbrdf) * abs(wi.y);

		}
		else
		{
			w.set(0);
		}

		return w;
	}

	float pdf(float3 wo, float3 wi)
	{
		if (!onSameHemisphere(wo, wi)) return 0.f;
		
		return pdfGGXReflectionConductor(wo, wi, roughness2.x, roughness2.y);
	}

	float getEnergyLeftAfterLayer(float3 wo, float3 wi, float layerWeight)
	{
		return (1.f - layerWeight);
	}


	SpectralSamples color0;
	SpectralSamples color1;
	float2 roughness2;
	float linearRoughness;
	float fromIOR;
};

struct DiffuseLayer : MaterialLayer
{
	static DiffuseLayer init(SpectralSamples color, float2 a2)
	{
		DiffuseLayer l;
		l.roughness2 = a2;
		l.color = color;
		return l;
	}

	float3 sampleWi(float3 wo, float rand)
	{
		return sampleDiffuseLambertian(roughness2.x, roughness2.y, wo, rand);
	}

	SpectralSamples evaluate(float3 wo, float3 wi)
	{
		SpectralSamples w;

		if (onSameHemisphere(wo, wi))
		{
			w = evaluateDiffuseLambertian(color, roughness2.x, roughness2.y, wo, wi) * abs(wi.y);
		}
		else
		{
			w.set(0);
		}

		return w;
	}

	float pdf(float3 wo, float3 wi)
	{
		if (!onSameHemisphere(wo, wi)) return 0.f;
		return pdfDiffuseLambertian(wo, wi, roughness2.x, roughness2.y);

	}

	float getEnergyLeftAfterLayer(float3 wo, float3 wi, float layerWeight)
	{
		return 1.f - layerWeight;
	}

	SpectralSamples color;
	float2 roughness2;
};

struct TransmittedLayer : MaterialLayer
{

	static TransmittedLayer init(float2 a2, float linearRoughness, float eta)
	{
		TransmittedLayer l;
		l.roughness2 = a2;
		l.linearRoughness = linearRoughness;
		l.etaR = eta;

		return l;
	}

	float3 sampleWi(float3 wo, float rand)
	{
		return sampleGGXTransmitted(etaR, roughness2.x, roughness2.y, wo, rand);
	}

	SpectralSamples evaluate(float3 wo, float3 wi)
	{
		SpectralSamples w;

		if (!onSameHemisphere(wo, wi))
		{
			float weight = evaluateGGXTransmitted(etaR, roughness2.x, roughness2.y, wo, wi);

			float msbrdf = getEnergyCompensationTranslucent(etaR, wo.y, wi.y, linearRoughness, weight);
			weight += msbrdf;
			weight *= abs(wi.y);

			w.set(weight);
		}
		else
		{
			w.set(0);
		}

		return w;
	}

	float pdf(float3 wo, float3 wi)
	{
		if (onSameHemisphere(wo, wi)) return 0.f;
		return pdfGGXTransmitted(etaR, wo, wi, roughness2.x, roughness2.y);

	}

	float getEnergyLeftAfterLayer(float3 wo, float3 wi, float layerWeight)
	{
		return 0;
	}

	float2 roughness2;
	float linearRoughness;
	float etaR;

};


//utilities
template<typename MATERIALTYPE>
SpectralSamples evaluateLayer(in MATERIALTYPE mat, in float3 wo, in float3 wi, in float layerWeight)
{
	SpectralSamples weight = mat.evaluate(wo, wi);
	weight = weight * layerWeight;
	return weight;
}



#endif