#ifndef MISCBRDF_HLSL_INCL
#define MISCBRDF_HLSL_INCL
#include "commonMath.hlsl"


float3 sampleHemisphere(in float2 rand)
{
    float y = 1.f - rand.x;
    float r = sqrt(1.f - sqr(y));
    float phi = 2.f * PI * rand.y;
    float s, c;
    sincos(phi, s, c);
    return float3(r * c, y,  r * s);
}

float3 sampleCosineWeightedHemisphere(in float2 rand)
{
    float y = sqrt(1.f - rand.x);
    float r = sqrt(rand.x);
    float phi = 2.0f * PI * rand.y;
    float s, c;
    sincos(phi, s, c);
    
    float x = r * c;
    float z = r * s;
    return float3(x, y, z);
}

float3 sampleCosineWeightedSphere(in float2 s)
{
	//TODO: should probably pick the hemisphere with 3rd uncorrelated rand value, rather than scaling it.

    bool flip = s.x < 0.5f;

    s.x = flip ? s.x * 2.f : (s.x - 0.5f) * 2.f;

    float3 dir = sampleCosineWeightedHemisphere(s);
    dir.y = flip ? -dir.y : dir.y;
    return dir;
}

//cos^(exponent+1) weighted hemisphere. exponent == 0 is identical to sampleCosineWeightedHemisphere
float3 sampleCosinePowerWeightedHemisphere(in float2 rand, in float exponent)
{
    float y = pow(1.f - rand.x, 1.f / (exponent + 2.f));
    float r = sqrt(max(0.f, 1.f - sqr(y)));
    float phi = 2.0f * PI * rand.y;
    float s, c;
    sincos(phi, s, c);

    float x = r * c;
    float z = r * s;
    return float3(x, y, z);
}

float3 sampleCosinePowerWeightedSphere(in float2 s, in float exponent)
{
	//TODO: should probably pick the hemisphere with 3rd uncorrelated rand value, rather than scaling it.

    bool flip = s.x < 0.5f;

    s.x = flip ? s.x * 2.f : (s.x - 0.5f) * 2.f;

    float3 dir = sampleCosinePowerWeightedHemisphere(s, exponent);
    dir.y = flip ? -dir.y : dir.y;
    return dir;
}


float3 sampleSphere(in float2 rand)
{
    float y = 1 - 2 * rand.x;
    float r = sqrt(1.f - sqr(y));
    float phi = 2.f * PI * rand.y;
    float s, c;
    sincos(phi, s, c);
    return float3(r * c, y,  r * s);
}

float pdfCosineWeightedHemisphere(in float dotIN)
{
    return saturate(dotIN) * INVPI;

}

float pdfCosineWeightedSphere(in float dotIN)
{
    return abs(dotIN) * INVPI * 0.5f;

}

float pdfCosinePowerWeightedHemisphere(in float dotIN, in float exponent)
{
    return (exponent + 2.f) * INVPI * 0.5f * pow(saturate(dotIN), exponent + 1.f);

}

float pdfCosinePowerWeightedSphere(in float dotIN, in float exponent)
{
    return (exponent + 2.f) * INVPI * 0.25f * pow(abs(dotIN), exponent + 1.f);

}

//angular emission profile of an emissive surface: Le(w) = emissive * (n+2)/2 * |cos|^n.
//the (n+2)/2 factor keeps the total emitted power constant as the exponent grows, so increasing the
//focus tightens the lobe instead of dimming the scene. exponent == 0 returns 1 (lambertian emitter).
//sidedness is deliberately not handled here, the callers do their own one sided culling.
float evaluateEmissionProfile(in float dotIN, in float exponent)
{
	//pow(0, 0) is not well defined, and the lambertian case is the common one, so branch it out
    float profile = exponent > 0.f ? pow(abs(dotIN), exponent) : 1.f;
    return (exponent + 2.f) * 0.5f * profile;

}

float pdfHemisphere()
{
    return 1.f / (2.f * PI);
}

float pdfSphere()
{
    return 1.f / (4.f * PI);
}

float3 evaluateLambertian(in float3 albedo, in float3 wi) 
{
	return albedo / PI;
}

//from imageworks sheen: http://www.aconty.com/pdf/s2017_pbs_imageworks_sheen.pdf
float GLSheen(float x, float a, float b, float c, float d, float e)
{
	return (a/(1+b*pow(x, c))) + d * x + e;
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
	if(cosTheta < 0.5f)
	{
		exponent = GLSheen(cosTheta, a, b, c, d, e);
	}
	else
	{
		exponent = 2.f * GLSheen(0.5f, a, b, c, d, e) - GLSheen(1.f - cosTheta, a, b, c, d, e);
	}
	return exp(exponent);
}

float DSheen( in float3 wm, in float r) 
{
	float rInv = 1.f/r;
    float sinTheta2 = max(1.f - wm.y * wm.y, 0.0001f);
	float v = (2.f + rInv) * pow(sinTheta2, rInv * 0.5f);
	return v / (2.f*PI);
}

float GSheen(in float3 wo, in float3 wi, in float r) 
{

	float lambdaI = GLambdaSheen(r, wi.y); //apply the "light side softening"? breaks reciprocity
	float lambdaO = GLambdaSheen(r, wo.y);
	
	return 1.f/(1.f + lambdaI + lambdaO);
}

#endif