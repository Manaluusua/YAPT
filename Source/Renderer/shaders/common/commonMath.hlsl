#ifndef COMMON_MATH_HLSL_INCL
#define COMMON_MATH_HLSL_INCL
#include "common.hlsl"

#define PI 3.14159265f
#define PI_OVER_2 1.57079633f
#define PI_OVER_4 0.785398163f

float sqr(float x)
{
    return x * x;
}

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
	float dotMI = dot(wi, wm);
	
	float denom = sqr(dotMO / eta + dotMI);
    return safeDiv(abs(dotMI), denom);
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

float2 sampleConcentricDisk(float2 u)
{
	float2 unitSquare = 2.f * u - float2(1, 1);

	if (unitSquare.x == 0 && unitSquare.y == 0)
		return float2(0, 0);

	float theta, r;
	if (abs(unitSquare.x) > abs(unitSquare.y)) {
		r = unitSquare.x;
		theta = PI_OVER_4 * (unitSquare.y / unitSquare.x);
	}
	else {
		r = unitSquare.y;
		theta = PI_OVER_2 - PI_OVER_4 * (unitSquare.x / unitSquare.y);
	}

	float cosTheta;
	float sinTheta;
	sincos(theta, sinTheta, cosTheta);

	return r * float2(cosTheta, sinTheta);
}


void constructVectorBase(float3 v, out float3 v2Out, out float3 v3Out) {
	if (abs(v.x) > abs(v.y))
		v2Out = float3(-v.z, 0, v.x) / sqrt(v.x * v.x + v.z * v.z);
	else
		v2Out = float3(0, v.z, -v.y) / sqrt(v.y * v.y + v.z * v.z);
	v3Out = cross(v, v2Out);
}

float3 makeOrthogonal(in float3 orthogonalTo, in float3 v)
{
    return normalize(v - orthogonalTo * (dot(orthogonalTo, v)));
}

float3x3 constructBasisTransform(in float3 n, float3 t)
{
    t = makeOrthogonal(n, t);
    float3 b = cross(t, n);
    return float3x3(t, n, b);
}

float gaussian(float x, float sigma)
{
    return exp(-(x * x) / (2.0 * sigma * sigma));
}

float gaussian(float2 xy, float sigma)
{
    float denom = 1.f / (2.0 * sigma * sigma);
    float x2 = xy.x * xy.x;
    float y2 = xy.y * xy.y;
    return exp(-(x2 + y2) * denom);
}

#endif