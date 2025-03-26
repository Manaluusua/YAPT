#ifndef COMMON_MATH_HLSL_INCL
#define COMMON_MATH_HLSL_INCL
#include "common.hlsl"

#define PI 3.14159265
#define sqr(x) (x*x)


float3 refr(float etaR, float3 wm, float3 wo)
{
	float c = dot(wm, wo);
	float temp = 1 + etaR * (sqr(c) - 1.f);
	if(temp < 0) //TIR
	{
		return 0.f;
	}
	
	temp = etaR * c - sign(wo.y) * sqrt(temp);
	return normalize(temp * wm - etaR * wo);
}



float jReflection(float3 wi, float3 wm)
{
	return safeDiv(1.f, 4 * saturate(dot(wi,wm)));
}


float jRefraction(float eta, float3 wo, float3 wm, float3 wi)
{
	float dotMO = dot(wo, wm);
	float dotMI = abs(dot(wi, wm));
	
	float denom = sqr(dotMO + eta * dotMI);
    return safeDiv(sqr(eta) * dotMI, denom);
}



float calculateTransmittance(float distance, float absorption)
{
	return exp(-absorption * distance);
}

bool onSameHemisphere(float3 referenceDir, float3 dir)
{
	return referenceDir.y * dir.y >= 0;
}

float3 forceSameHemisphere(float3 referenceDir, float3 dir)
{
	return onSameHemisphere(referenceDir, dir) ? dir : -dir;
}

#endif