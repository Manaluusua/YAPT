
#ifndef BSDFSAMPLE_HLSL_INCL
#define BSDFSAMPLE_HLSL_INCL
#include "../utils/ggx.hlsl"
#include "../utils/miscBrdf.hlsl"
#include "hitShadersCommon.hlsl"
#include "../utils/multiScatter.hlsl"



///////////////////////////////////////////////////////////////////////////////////////////////// 
float3 sampleGGXReflectionConductor(in float ax ,in float ay, in float3 wo, in float3 sample)
{
	float3 wm = sampleWMGGX(wo, ax, ay, sample.x, sample.y);
	float3 wi = reflect(-wo, wm);
	if(wi.y < 0.f)
	{
		return 0.f;
	}
	return wi;
}

float pdfGGXReflectionConductor(in float3 wo, in float3 wi, in float ax, in float ay)
{
	float3 wm = normalize(wo + wi);
	if(dot(wm, wo) < 0.f)
	{
		return 0.f;
	}
	return pdfWMGGX(wo, wm, ax, ay) * jReflection(wo, wm);
}

float3 evaluateGGXReflectionConductor(in float3 etaR, in float3 etaK,in float ax ,in float ay, in float3 wo, in float3 wi)
{
	float3 wm = normalize(wo + wi);
	if(wi.y < 0.0f || dot(wi, wm) < 0.f)
	{
		return 0.f;
	}

	float3 F = fresnelDielectricConductor(etaR, etaK, dot(wo, wm));
	float G2 = G2GGX(wo, wi, wm, ax, ay);
	float D = DGGX(wm, ax, ay);
	
	return F * G2 * D / max(4.f * wo.y * wi.y, 0.00001f);
}

///////////////////////////////////////////////////////////////////////////////////////////////// 
float3 sampleGGXReflectionDielectric(in float ax ,in float ay, in float3 wo, in float3 sample)
{
	float3 wm = sampleWMGGX(wo, ax, ay, sample.x, sample.y);
	float3 wi = reflect(-wo, wm);
	if(wi.y < 0.f)
	{
		return 0.f;
	}
	return wi;
}

float pdfGGXReflectionDielectric(in float3 wo, in float3 wi, in float ax, in float ay)
{
	if(wi.y < 0.0f)
	{
		return 0.f;
	}
	
    float3 wm = normalize(wo + wi);	
	float pdf = pdfWMGGX(wo, wm, ax, ay) * jReflection(wo, wm);
	return pdf;
}

float3 evaluateGGXReflectionDielectric(in float etaR,in float ax ,in float ay, in float3 wo, in float3 wi)
{
	float3 wm = normalize(wo + wi);
	if(wi.y < 0.0f)
	{
		return 0.f;
	}
	
	if( dot(wi, wm) > 0.f && dot(wo, wm) > 0)
	{
		float F = fresnelDielectricDielectric2(etaR, dot(wo, wm));

		float D = DGGX(wm, ax, ay);
		float G2 = G2GGX(wo, wi, wm, ax, ay);
		return F * G2 * D / max(4.f * wo.y * wi.y, 0.00001f);
	} 
	else
	{
		return 0.f;
	}

}


/////////////////////////////////////////////////////////////////////////////////////////////////////

float3 getWMTranslucent(in float3 wo,in float3 wi, in float etaR)
{
	float3 wm;
	bool isReflected = onSameHemisphere(wo, wi);
	if(isReflected)
	{
		wm = normalize(wo + wi); 
	}
	else
	{
		if(etaR == 1.f)
		{
			etaR = 1.001f;
		}
		
		wm = normalize(wo + wi * etaR);
		wm = forceSameHemisphere(wo, wm);
	}
	
	return wm;
}

float3 sampleGGXTransmitted(in float etaR, in float ax ,in float ay, in float3 wo, in float3 sample)
{
	float3 wm;
	float3 wi;

	wm = sampleWMGGX(wo, ax, ay, sample.x, sample.y);
	
	if(dot(wo, wm) < 0.f)
	{
		return 0.f;
	}
	
	if(etaR == 1.f)
	{
		etaR = 1.001f;
	}

	float invEta = 1.f/etaR;
	wi = refract(-wo, wm, invEta);
	
	return wi;
}

float pdfGGXTransmitted(in float etaR, in float3 wo, in float3 wi, in float ax, in float ay)
{
	float3 wm;
	
	if(etaR == 1.f)
	{
		etaR = 1.001f;
	}
	
	wm = getWMTranslucent(wo, wi, etaR);
	
	bool isReflected = onSameHemisphere(wo, wi);
	
	if(isReflected)
	{
		return 0.f;
	}

	float pdf = pdfWMGGX(wo, wm, ax, ay);

	if (dot(wi, wm) > 0) return 0.f;
	if (dot(wo, wm) * dot(wi, wm) > 0) return 0.f;
	
	pdf *= jRefraction(etaR, wo, wm, wi);
	return pdf;
	
	
}

float3 evaluateGGXTransmitted(in float etaR, in float ax, in float ay, in float3 wo, in float3 wi)
{
	if(etaR == 1.f)
	{
		etaR = 1.001f;
	}
	
	float3 wm = getWMTranslucent(wo, wi, etaR);
	
	bool isReflected = onSameHemisphere(wo, wi);
	
	if(isReflected)
	{
		return 0.f;
	}
	
	if (dot(wo, wm) * dot(wi, wm) > 0) return 0.f;
	
	float3 weight;

	float VdotH = saturate(dot(wo, wm));
	
	float3 F = fresnelDielectricDielectric2(etaR, VdotH);
	float G2 = G2GGX(wo, wi, wm, ax, ay);
	float D = DGGX(wm, ax, ay);
	
	weight = (1.f - F) * G2 * D * VdotH * jRefraction(etaR, wo, wm, wi) / SAFE_DIVISOR(abs(wo.y * wi.y));
	

	return weight;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
float3 sampleDiffuseLambertian(in float ax ,in float ay, in float3 wo, in float3 sample)
{
	float3 wi = sampleHemisphere(sample.xy);
	if(wi.y < 0.f)
	{
		return 0.f;
	}
	return wi;

}

float pdfDiffuseLambertian(in float3 wo, in float3 wi, in float ax, in float ay)
{
	if(wi.y < 0.0f)
	{
		return 0.f;
	}
	float pdf = pdfHemisphere(wi); //diff
	return pdf;
}

float3 evaluateDiffuseLambertian(in float3 albedo, in float ax, in float ay, in float3 wo, in float3 wi)
{
	float3 w = evaluateLambertian(albedo, wi);
	return w;
}


/////////////////////////////////////////////////////////////////////////////////////////////////////
float3 sampleSheen(in float r, in float3 wo, in float3 sample)
{
	float3 wi;
	wi = sampleHemisphere(sample.xy);
	if(wi.y < 0.f)
	{
		return 0.f;
	}
	return wi;

}

float pdfSheen(in float3 wo, in float3 wi, in float r)
{
	if(wi.y < 0.0f)
	{
		return 0.f;
	}
	float pdf =  pdfHemisphere(wi);
	return pdf;
}

float3 evaluateSheen(in float3 sheenColor, in float r, in float3 wo, in float3 wi)
{
	float3 colOut = 0.f;
	if(wi.y < 0.0f)
	{
		return colOut;
	}
	
	float3 wm = normalize(wo + wi);
	
	if( dot(wi, wm) > 0.f && dot(wo, wm) > 0)
	{
		//replace with schlick?
		float3 F = sheenColor;//fresnelDielectricDielectric2(1.5f, dot(wo, wm)) * sheenColor;

		float D = DSheen(wm, r);
		float G = GSheen(wo, wi, r);
		colOut = F * G * D / max(4.f * wo.y * wi.y, 0.00001f);
	} 
	return colOut;
}



#endif