#ifndef GGX_HLSL_INCL
#define GGX_HLSL_INCL

#include "commonMath.hlsl"
#include "surfaceParameters.hlsl"
#include "fresnel.hlsl"


#define USE_VISIBLE_NORMALS

float smithLambdaGGX(float3 d, float ax, float ay)
{
	float x2 = d.x * d.x;
	float ax2 = ax * ax;
	float y2 = d.z * d.z;
	float ay2 = ay * ay;
	
	float s = 1.0f + (x2 * ax2 + y2 * ay2) / (d.y * d.y);
	return (sqrt(s) - 1.f) * 0.5f;
}


float G1GGX(float3 wo, float3 wm, float ax, float ay)
{
   float lambda = smithLambdaGGX(wo, ax, ay);
   return (dot(wo, wm) >= 0.f ? 1.f : 0.f ) / (1.0f + lambda);
}

//Height correlated G2
float G2GGX(float3 wo, float3 wi, float3 wm, float ax, float ay)
{
    float lambdaI = smithLambdaGGX(wi, ax, ay);
	float lambdaO = smithLambdaGGX(wo, ax, ay);
	
	float nom = 1.f; //(dot(wo, wm) >= 0.f ? 1.f : 0.f ) * (dot(wi, wm) >= 0.f ? 1.f : 0.f );
	float denom = 1 + lambdaI + lambdaO;
	
	return nom/denom;
}

float3 sampleNormalsGGX(float ax, float ay, float u1, float u2)
{
	float3 t1 = float3(1.f, 0.0f, 0.0f);
	float3 t2 = float3(0.f, 0.f, 1.f);
	float3 wm = sqrt(u1 / SAFE_DIVISOR(1.f - u1)) * (ax * cos(2.f * PI * u2) * t1 + ay * sin(2.f * PI * u2) * t2);
	wm += float3(0.f, 1.f, 0.f);
	return normalize(wm);
}

float DGGX(float3 wm, float ax, float ay)
{
	float denom = (sqr(wm.x / ax) + sqr(wm.z / ay) + sqr(wm.y));
	denom *= denom;
	denom *= ax * ay * PI;
	return safeDiv(1.f, denom);
	
}


//Sampling the GGX Distribution of Visible Normals by Eric Heitz
//Difference here to the article is that we use y up, not z up
float3 sampleVisibleNormalsGGX(float3 wo, float ax, float ay, float u1, float u2)
{

	// Section 3.2: transforming the view direction to the hemisphere configuration
	float3 Vh = normalize(float3(ax * wo.x, ay * wo.z,  wo.y));
	// Section 4.1: orthonormal basis (with special case if cross product is zero)
	float lensq = Vh.x * Vh.x + Vh.y * Vh.y;
	float3 T1 = lensq > 0 ? float3(-Vh.y, Vh.x, 0) * (1.f/sqrt(lensq)) : float3(1,0,0);
	float3 T2 = cross(Vh, T1);
	// Section 4.2: parameterization of the projected area
	float r = sqrt(u1);
	float phi = 2.0 * PI * u2;
	float t1 = r * cos(phi);
	float t2 = r * sin(phi);
	float s = 0.5 * (1.0 + Vh.z);
	t2 = (1.0 - s)*sqrt(1.0 - t1*t1) + s*t2;
	// Section 4.3: reprojection onto hemisphere
	float3 Nh = t1*T1 + t2*T2 + sqrt(max(0.0, 1.0 - t1*t1 - t2*t2))*Vh;
	// Section 3.4: transforming the normal back to the ellipsoid configuration
	float3 Ne = normalize(float3(ax * Nh.x, max(0.0, Nh.z), ay * Nh.y));

	
	return Ne;
}

float DVGGX(float3 wo, float3 wm, float ax, float ay)
{
	float D = DGGX(wm, ax, ay);
	float G1 = G1GGX(wo, wm, ax, ay);
	return safeDiv(G1 * abs(dot(wo, wm)) * D, abs(wo.y)); 
}

float3 sampleWMGGX(float3 wo, float ax, float ay, float u1, float u2)
{
	bool flipWo = wo.y < 0;
	wo.y = abs(wo.y);

#ifdef USE_VISIBLE_NORMALS
	float3 wh = sampleVisibleNormalsGGX(wo, ax, ay, u1, u2);
#else
	float3 wh = sampleNormalsGGX(ax, ay, u1, u2);
#endif

	if(flipWo)
	{
		wo.y = -wo.y;
		wh.y = -wh.y;
	}
	return forceSameHemisphere(wo, wh);

}

float pdfWMGGX(float3 wo, float3 wm, float ax, float ay)
{
#ifdef USE_VISIBLE_NORMALS
	return DVGGX(wo, wm, ax, ay);
#else
	return DGGX(wm, ax, ay) * abs(wm.y);
#endif

}

#endif
