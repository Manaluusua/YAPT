
#ifndef BSDFSAMPLE_HLSL_INCL
#define BSDFSAMPLE_HLSL_INCL
#include "ggx.hlsl"
#include "miscBrdf.hlsl"



///////////////////////////////////////////////////////////////////////////////////////////////// 
float3 sampleGGXReflectionConductor(in float ax ,in float ay, in float3 wo, in float2 s)
{
	float3 wm = sampleWMGGX(wo, ax, ay, s.x, s.y);
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

template<typename T>
T evaluateGGXReflectionConductor(in T etaR, in T etaK,in float ax ,in float ay, in float3 wo, in float3 wi)
{
	float3 wm = normalize(wo + wi);
	if(!onSameHemisphere(wo, wi))
	{
		return (T)0;
	}

	T F = fresnelDielectricConductor(etaR, etaK, dot(wo, wm));
	float G2 = G2GGX(wo, wi, wm, ax, ay);
	float D = DGGX(wm, ax, ay);
	
	return F * G2 * D / max(4.f * wo.y * wi.y, 0.00001f);
}

///////////////////////////////////////////////////////////////////////////////////////////////// 
float3 sampleGGXReflectionDielectric(in float ax ,in float ay, in float3 wo, in float2 s)
{
	float3 wm = sampleWMGGX(wo, ax, ay, s.x, s.y);
	float3 wi = reflect(-wo, wm);
	if(!onSameHemisphere(wo, wi))
	{
		return 0.f;
	}
	return wi;
}

float pdfGGXReflectionDielectric(in float3 wo, in float3 wi, in float ax, in float ay)
{
	if(!onSameHemisphere(wo, wi))
	{
		return 0.f;
	}
	
    float3 wm = normalize(wo + wi);	
	float pdf = pdfWMGGX(wo, wm, ax, ay) * jReflection(wo, wm);
	return pdf;
}

float evaluateGGXReflectionDielectric(in float etaR, in float ax ,in float ay, in float3 wo, in float3 wi)
{
	float3 wm = normalize(wo + wi);
	if(!onSameHemisphere(wo, wi))
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

float3 sampleGGXTransmitted(in float etaR, in float ax ,in float ay, in float3 wo, in float2 s)
{
	float3 wm;
	float3 wi;

	wm = sampleWMGGX(wo, ax, ay, s.x, s.y);
	
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

	if (dot(wo, wm) * dot(wi, wm) > 0) return 0.f;
	
	pdf *= jRefraction(etaR, wo, wm, wi);
	return pdf;
	
	
}

float evaluateGGXTransmitted(in float etaR, in float ax, in float ay, in float3 wo, in float3 wi)
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
	
	float weight;

	float VdotH = abs(dot(wo, wm));
	
	float F = fresnelDielectricDielectric2(etaR, VdotH);
	float G2 = G2GGX(wo, wi, wm, ax, ay);
	float D = DGGX(wm, ax, ay);
	
	float denom = abs(wo.y * wi.y);

	weight = (1.f - F) * G2 * D * VdotH * jRefraction(etaR, wo, wm, wi);
	weight = safeDiv(weight, denom);
	

	return weight;
}

///////////////////////////////////////////////////////////////////////////////////////////////// 

float2 getReflAndRefrProbabilities(in float etaR, in float3 wo, in float3 wm, bool allowReflection, bool allowTransmission)
{
    float VdotH = abs(dot(wo, wm));
    float reflProb = fresnelDielectricDielectric2(etaR, VdotH);
    float refrProb = 1 - reflProb;
	
    if (!allowReflection)
    {
        reflProb = 0;
    }
    if (!allowTransmission)
    {
        refrProb = 0;
    }

    if (reflProb == 0 && refrProb == 0)
    {
        return 0;
    }
	
    float sum = reflProb + refrProb;
	
    reflProb = safeDiv(reflProb, sum);
    refrProb = safeDiv(refrProb, sum);
    return float2(reflProb, refrProb);
	
}

float3 sampleDielectric(float etaR, in float ax, in float ay, in float3 wo, in float2 s, in float sc, bool allowReflection, bool allowTransmission)
{
    float3 wm = sampleWMGGX(wo, ax, ay, s.x, s.y);
    if (dot(wo, wm) < 0.f)
    {
        return 0.f;
    }
	
    if (etaR == 1.f)
    {
        etaR = 1.001f;
    }
	
    float2 reflAndRefrProbabilities = getReflAndRefrProbabilities(etaR, wo, wm, allowReflection, allowTransmission);
	
    if (reflAndRefrProbabilities.x == 0 && reflAndRefrProbabilities.y == 0)
    {
        return 0;
    }
	
	//reflect
    if (sc < reflAndRefrProbabilities.x)
    {
        float3 wi = reflect(-wo, wm);
        if (!onSameHemisphere(wo, wi))
        {
            return 0.f;
        }
        return wi;
    } 
	else //refract
    {
        float invEta = 1.f / etaR;
        float3 wi = refract(-wo, wm, invEta);
        if (onSameHemisphere(wo, wi))
        {
            return 0.f;
        }
		return wi;
    }

}

float pdfDielectric(in float etaR, in float3 wo, in float3 wi, in float ax, in float ay, bool allowReflection, bool allowTransmission)
{
    float3 wm = getWMTranslucent(wo, wi, etaR);
    float2 reflAndRefrProbabilities = getReflAndRefrProbabilities(etaR, wo, wm, allowReflection, allowTransmission);
    float pdf;
    if (onSameHemisphere(wo, wi))
    {
        pdf = pdfGGXReflectionDielectric(wo, wi, ax, ay) * reflAndRefrProbabilities.x;

    }
    else
    {
        pdf = pdfGGXTransmitted(etaR, wo, wi, ax, ay) * reflAndRefrProbabilities.y;
    }
    return pdf;

}

/////////////////////////////////////////////////////////////////////////////////////////////////////
float3 sampleDiffuseLambertian(in float ax ,in float ay, in float3 wo, in float2 s)
{
	float3 wi = sampleHemisphere(s.xy);
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
	float pdf = pdfHemisphere(); //diff
	return pdf;
}
template<typename T>
T evaluateDiffuseLambertian(in T albedo, in float ax, in float ay, in float3 wo, in float3 wi)
{
	return albedo / PI;

}


/////////////////////////////////////////////////////////////////////////////////////////////////////
float3 sampleSheen(in float r, in float3 wo, in float2 s)
{
	float3 wi;
	wi = sampleHemisphere(s.xy);
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
	float pdf =  pdfHemisphere();
	return pdf;
}

template<typename T>
T evaluateSheen(in T sheenColor, in float r, in float3 wo, in float3 wi)
{
	T colOut = (T)0.f;
	if(wi.y < 0.0f)
	{
		return colOut;
	}
	
	float3 wm = normalize(wo + wi);
	
	if( dot(wi, wm) > 0.f && dot(wo, wm) > 0)
	{
		//replace with schlick?
		T F = sheenColor;//fresnelDielectricDielectric2(1.5f, dot(wo, wm)) * sheenColor;

		float D = DSheen(wm, r);
		float G = GSheen(wo, wi, r);
		float DGDenom = D * G / max(4.f * wo.y * wi.y, 0.00001f);
		colOut = F * DGDenom;
	} 
	return colOut;
}



#endif