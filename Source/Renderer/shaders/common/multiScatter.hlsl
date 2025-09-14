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

//-------------------------------------- Favg Fits (County & Kulla) and related utils -----------------------------------------------------//

float getDirectionalAlbedoTranslucent(in float etaR, in float dotDir, in float linearRoughness)
{
    if (etaR < 1.f)
    {
        return getSSDirectionalAlbedoTranslucentLighter(etaR, abs(dotDir), linearRoughness);
    }
    else
    {
        return getSSDirectionalAlbedoTranslucentDenser(etaR, abs(dotDir), linearRoughness);
    }
	
}

float getAverageAlbedoTranslucent(in float etaR, in float linearRoughness)
{
    if (etaR < 1.f)
    {
        return getSSAverageAlbedoTranslucentLighter(etaR, linearRoughness);
    }
    else
    {
        return getSSAverageAlbedoTranslucentDenser(etaR, linearRoughness);
    }
	
}

float getAvgFresnel(float etaR)
{
    float fAvg;
    if (etaR < 1)
    {
        fAvg = 0.997118f + 0.1014f * etaR - 0.965241f * etaR * etaR - 0.130607f * etaR * etaR * etaR;
    }
    else
    {
        fAvg = (etaR - 1.f) / (4.08567f + 1.00071f * etaR);
    }

    return fAvg;
}

float getReflectionRatio(float etaR, float linearRoughness)
{
    float a = getAvgFresnel(etaR);
    float b = getAverageAlbedoTranslucent(1.f / etaR, linearRoughness); //sampleLut(vec2p(linearRoughness, 1.f / etaR), etaR < 1.f ? singleScatteringAverageAlbedoDenserLut : singleScatteringAverageAlbedoLighterLut, vec2p(SS_ALBEDO_TRANSLUCENT_LUT_DIM, SS_ALBEDO_TRANSLUCENT_LUT_DIM));
    float c = getAvgFresnel(1.f / etaR);
    float d = getAverageAlbedoTranslucent(etaR, linearRoughness); //sampleLut(vec2p(linearRoughness, etaR), etaR < 1.f ? singleScatteringAverageAlbedoLighterLut : singleScatteringAverageAlbedoDenserLut, vec2p(SS_ALBEDO_TRANSLUCENT_LUT_DIM, SS_ALBEDO_TRANSLUCENT_LUT_DIM));
    float e = etaR * etaR;

    float x = (b - 1.f) * (c - 1.f) * e / SAFE_DIVISOR(a * d - a + b * c * e - b * e - c * e - d + e + 1.f);
    return x;
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
	return getEnergyCompensationKulla(fms, abs(dotWo), abs(dotWi), linearRoughness, singleScatter);
	//return getEnergyCompensationTurquin(fms, abs(dotWo), abs(dotWi), linearRoughness, singleScatter);
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

float getEnergyCompensationTranslucentKulla(in float etaR, in float dotWo, in float dotWi, in float linearRoughness, in float singleScatter)
{
    bool transmission = dotWo * dotWi < 0;
	
    float dirAlbedoWo;
    float dirAlbedoWi;
    float eAvg;
    float ratio;
	
	if(transmission)
    {
        float etaInv = 1.f / etaR;
        dirAlbedoWo = getDirectionalAlbedoTranslucent(etaR, dotWo, linearRoughness);
        dirAlbedoWi = getDirectionalAlbedoTranslucent(etaInv, dotWi, linearRoughness);
        eAvg = getAverageAlbedoTranslucent(etaInv, linearRoughness);
        ratio = getReflectionRatio(etaR, linearRoughness);
    } 
	else
    {
        dirAlbedoWo = getDirectionalAlbedoTranslucent(etaR, dotWo, linearRoughness);
        dirAlbedoWi = getDirectionalAlbedoTranslucent(etaR, dotWi, linearRoughness);
        eAvg = getAverageAlbedoTranslucent(etaR, linearRoughness);
        ratio = getReflectionRatio(etaR, linearRoughness);
    }
	
	//TODO: handle transmission vs reflection. now assumes transmission
    
	
    float ems = (ratio * (1.f - dirAlbedoWo) * (1.f - dirAlbedoWi)) / max(0.00001f, PI - eAvg);
	return ems;
}


float getEnergyCompensationTranslucentTurquin(in float etaR, in float dotWo, in float dotWi, in float linearRoughness, in float singleScatter)
{
    float dirAlbedoWo;
    if (etaR < 1.f)
    {
        dirAlbedoWo = getSSDirectionalAlbedoTranslucentLighter(etaR, abs(dotWo), linearRoughness);
    }
    else
    {
        dirAlbedoWo = getSSDirectionalAlbedoTranslucentDenser(etaR, abs(dotWo), linearRoughness);
    }

    float ems = (singleScatter / dirAlbedoWo) - singleScatter;

    return ems;
}

float getEnergyCompensationTranslucent(in float etaR, in float dotWo, in float dotWi, in float linearRoughness, in float singleScatter)
{
    return getEnergyCompensationTranslucentTurquin(etaR, dotWo, dotWi, linearRoughness, singleScatter);

}

#endif