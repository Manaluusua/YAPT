
#ifndef FRESNEL_HLSL_INCL
#define FRESNEL_HLSL_INCL

#include "commonMath.hlsl"

//metallic fresnel using incident and grazing angle parametrization for complex IOR. From: "Artist Friendly Metallic Fresnel" by Ole Gulbrandsen
float getConductorRefractiveIndex ( float r , float g )
{
	float sqrtR = sqrt(r);
	return  g * ((1-r)/(1+r)) + (1-g) * ((1+sqrtR)/(1-sqrtR)) ;
}
float getConductorExtinctionthroughputSquared (float r, float refractiveIndex){
	float nr = (refractiveIndex + 1) * (refractiveIndex + 1) * r - (refractiveIndex - 1.0f)*(refractiveIndex - 1.0f);
	return nr / (1.0f - r);
}


float2 getConductorRefractiveIndexAndExtinctionthroughputSquared(float r, float g)
{
	float rc = clamp (r, 0.0, 0.99);
	float n = getConductorRefractiveIndex(rc, g);
	float k2 = getConductorExtinctionthroughputSquared(rc, n);
	
	return float2(n, k2);
}



//from: https://seblagarde.wordpress.com/2013/04/29/memo-on-fresnel-equations/
template<typename T>
T fresnelDielectricConductor(T etaReal, T etaImg, float cosTheta)
{  
   float cosTheta2 = cosTheta * cosTheta;
   float sinTheta2 = 1 - cosTheta2;
   T etaReal2 = etaReal * etaReal;
   T etaImg2 = etaImg * etaImg;

   T t0 = etaReal2 - etaImg2 - sinTheta2;
   T a2plusb2 = sqrt(etaReal2 * etaImg2 * 4 + t0 * t0);
   T t1 = a2plusb2 + cosTheta2;
   T a = sqrt((a2plusb2 + t0) * 0.5f);
   T t2 = a * cosTheta2 * 2;
   T rs = (t1 - t2) / (t1 + t2);

   T t3 = a2plusb2 + sinTheta2 * sinTheta2 * cosTheta2;
   T t4 = t2 * sinTheta2;
   T rp = rs * (t3 - t4) / (t3 + t4);

   return (rp + rs) * 0.5f;
}

float fresnelDielectricDielectric1(float eta, float cosTheta)
{
   float sinTheta2 = 1 - cosTheta * cosTheta;

   float t0 = sqrt(1 - (sinTheta2 / (eta * eta)));
   float t1 = eta * t0;
   float t2 = eta * cosTheta;

   float rs = (cosTheta - t1) / (cosTheta + t1);
   float rp = (t0 - t2) / (t0 + t2);

   return 0.5 * (rs * rs + rp * rp);
}

float fresnelDielectricDielectric2(float eta, float cosTheta)
{
   float c = saturate(cosTheta);
   float temp = eta* eta + c * c - 1.f;

   if (temp < 0.f)
      return 1.f;

   float g = sqrt(temp);
   return 0.5f * sqr((g - c) / max(g + c, 0.00001f)) *
                       (1 + sqr(( (g + c)  * c - 1.f) / ((g - c) * c+ 1.f)));
}

float reflectivityFromIOR(float ior)
{
	return sqr(ior - 1) / sqr(ior + 1);
}

float3 fresnelSchlick(float3 reflectivity, float cosTheta)
{
    return reflectivity + (1.0f - reflectivity) * pow(1.0f - cosTheta, 5);
}

float3 fresnelSchlick(float3 reflectivity, float3 edgeTint, float cosTheta)
{
    return reflectivity + (edgeTint - reflectivity) * pow(1.0f - cosTheta, 5);
}

#endif