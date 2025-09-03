#ifndef MISCBRDF_HLSL_INCL
#define MISCBRDF_HLSL_INCL
#include "commonMath.hlsl"


float3 sampleHemisphere(in float2 s)
{
	float z = s.x;
    float r = sqrt(1.f - sqr(z));
    float phi = 2.f * PI * s.y;
    return float3(r * cos(phi), z,  r * sin(phi));
}

float pdfHemisphere()
{
    return 1.f / (2.f * PI);
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
	float sinTheta2 = 1.f - wm.y * wm.y;
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