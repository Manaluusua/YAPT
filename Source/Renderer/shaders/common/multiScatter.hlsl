#ifndef MULTISCATTER_HLSL_INCL
#define MULTISCATTER_HLSL_INCL
#include "commonMath.hlsl"
#include "fresnel.hlsl"

//-------------------------------------- LUT lookups -----------------------------------------------------//
float getAverageSSDirectionalAlbedoNoFresnel(float linearRoughness)
{
	return sampleLUT(g_lutSampler, g_avgDirAlbedoGGXNoFresnelLUT, linearRoughness).x;
}

float getSSDirectionalAlbedoNoFresnel(float cosTheta, float roughness)
{
	return sampleLUT(g_lutSampler, g_dirAlbedoGGXNoFresnelLUT, float2(cosTheta, roughness)).x;
}

float getSSMSDirectionalAlbedo(float eta, float cosTheta, float roughness)
{
	//eta in LUT [1,4]
	float etaLUT = saturate( (eta - 1.f) * 0.333333);
	return sampleLUT(g_lutSampler, g_dirAlbedoGGXSingleAndMultiScatterLUT, float3(cosTheta, roughness, etaLUT)).x;
}

float getSSMSAverageDirectionalAlbedo(float eta, float roughness)
{
	//eta in LUT [1,4]
	float etaLUT = saturate( (eta - 1.f) * 0.333333);
	return sampleLUT(g_lutSampler, g_avgDirAlbedoGGXSingleAndMultiScatterLUT, float2(roughness, etaLUT)).x;
}

float getSSDirectionalAlbedoTranslucentLighter(float eta, float cosTheta, float roughness)
{
	float etaLUT = saturate( (eta - 0.3f) * (1.f/0.7f));
	return sampleLUT(g_lutSampler, g_dirAlbedoGGXTranslucentLighterLUT, float3(cosTheta, roughness, etaLUT)).x;
}

float getSSDirectionalAlbedoTranslucentDenser(float eta, float cosTheta, float roughness)
{
	float etaLUT = saturate( (eta - 1.f) * 0.333333);
	return sampleLUT(g_lutSampler, g_dirAlbedoGGXTranslucentDenserLUT, float3(cosTheta, roughness, etaLUT)).x;
}


float getSSAverageAlbedoTranslucentLighter(float eta, float roughness)
{
	float etaLUT = saturate( (eta - 0.3f) * (1.f/0.7f));
	return sampleLUT(g_lutSampler, g_avgAlbedoGGXTranslucentLighterLUT, float2(roughness, etaLUT)).x;
}

float getSSAverageAlbedoTranslucentDenser(float eta, float roughness)
{
	float etaLUT = saturate( (eta - 1.f) * 0.333333);
	return sampleLUT(g_lutSampler, g_avgAlbedoGGXTranslucentDenserLUT, float2(roughness, etaLUT)).x;
}

float getSheenDirectionalAlbedo(float cosTheta, float roughness)
{
	roughness = saturate( roughness - 0.07f ); //minimum sheen roughness is 0.07
	return sampleLUT(g_lutSampler, g_dirAlbedoSheenNoFresnelLUT, float2(cosTheta, roughness)).x;
}


//-------------------------------------- Fms -----------------------------------------------------//
template<typename T>
T getFmsConductor(T etaR, T etaK, float cosTheta) //TODO
{
	return fresnelDielectricConductor(etaR, etaK, cosTheta);
}

float getFmsDielectric(float etaR, float cosTheta)
{
	return fresnelDielectricDielectric2(etaR, cosTheta);
}


//--------------------------------------Energy compensation -----------------------------------------------------//
template<typename T>
T getEnergyCompensationKulla(in T fms, in float dotWo, in float dotWi, in float linearRoughness, in T singleScattering)
{ 
	float dirAlbedoWo = getSSDirectionalAlbedoNoFresnel(abs(dotWo), linearRoughness);
	float dirAlbedoWi = getSSDirectionalAlbedoNoFresnel(abs(dotWi), linearRoughness);
	float eAvg = getAverageSSDirectionalAlbedoNoFresnel(linearRoughness);
	float ems = ((1.f - dirAlbedoWo) * (1.f - dirAlbedoWi)) / max(0.00001f, PI - eAvg);
	return  fms * ems;
	
}

template<typename T>
T getEnergyCompensationTurquin(in T fms, in float dotWo, in float dotWi, in float linearRoughness, in T singleScatter)
{
	float dirAlbedoWo = getSSDirectionalAlbedoNoFresnel(abs(dotWo), linearRoughness);
	T energyCompensation = fms * ( 1.f - dirAlbedoWo) / dirAlbedoWo;
	return singleScatter * energyCompensation;
}
template<typename T>
T getEnergyCompensation(in T fms, in float dotWo, in float dotWi, in float linearRoughness, in T singleScatter)
{
	//return getEnergyCompensationKulla(fms, abs(dotWo), abs(dotWi), linearRoughness, singleScatter);
	return getEnergyCompensationTurquin(fms, abs(dotWo), abs(dotWi), linearRoughness, singleScatter);
}

float getEnergyRemainingAfterSpecular(in float etaR, in float dotWo, in float dotWi, in float linearRoughness, in float specularAmount)
{
	float dirAlbedoWo = getSSMSDirectionalAlbedo(etaR, abs(dotWo), linearRoughness) * specularAmount;
	float dirAlbedoWi = getSSMSDirectionalAlbedo(etaR, abs(dotWi), linearRoughness) * specularAmount;
	float eAvg = getSSMSAverageDirectionalAlbedo(etaR, linearRoughness) * specularAmount;
	float ems = ((1.f - dirAlbedoWo) * (1.f - dirAlbedoWi)) / max(0.00001f, PI - eAvg);
	
	return ems * PI;
}

float getEnergyRemainingAfterSheen(in float dotWo, in float dotWi, in float linearRoughness, in float sheenAmount)
{
	float dirAlbedoWo = getSheenDirectionalAlbedo(abs(dotWo), linearRoughness) * sheenAmount;
	float dirAlbedoWi = getSheenDirectionalAlbedo(abs(dotWi), linearRoughness) * sheenAmount;
	float energyLeft = min(1.f - dirAlbedoWo, 1.f - dirAlbedoWi);
	
	return energyLeft;
}

float getEnergyCompensationTranslucent(in float etaR, in float dotWo, in float dotWi, in float linearRoughness, in float singleScatter)
{
	float dirAlbedoWo;
	if(etaR < 1.f)
	{
		dirAlbedoWo = getSSDirectionalAlbedoTranslucentLighter(etaR, abs(dotWo), linearRoughness);
	} else
	{
		dirAlbedoWo = getSSDirectionalAlbedoTranslucentDenser(etaR, abs(dotWo), linearRoughness);
	}

	float ems =  (singleScatter / dirAlbedoWo) - singleScatter;

	return ems;
}



#endif