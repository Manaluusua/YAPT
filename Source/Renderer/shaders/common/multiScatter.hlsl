#ifndef MULTISCATTER_HLSL_INCL
#define MULTISCATTER_HLSL_INCL
#include "commonMath.hlsl"
#include "fresnel.hlsl"
//#define RECIPROCAL_MULTISCATTER


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

float getReflRatioScaling(float etaR, float linearRoughness)
{
    float a = getAvgFresnel(etaR);
    float b = getAverageAlbedoTranslucent(1.f / etaR, linearRoughness);
    float c = getAvgFresnel(1.f / etaR);
    float d = getAverageAlbedoTranslucent(etaR, linearRoughness);
    float e = etaR * etaR;
    float h = safeDiv(1.f - d, 1.f - c) * (1.f / e);
    float x = safeDiv(1.f - b, 1.f - a) * safeDiv(1.f, h) + 1.f;
    return x;
}

//-------------------------------------- Fms -----------------------------------------------------//
template<typename T>
T getFmsConductor(T etaR, T etaK, float cosTheta) 
{
#ifdef RECIPROCAL_MULTISCATTER
    return fresnelDielectricConductor(etaR, etaK, cosTheta); //TODO: proper reciprocal term
#else    
	return fresnelDielectricConductor(etaR, etaK, cosTheta);
#endif
}

float getFmsDielectric(float etaR, float linearRoughness, float cosTheta)
{
#ifdef RECIPROCAL_MULTISCATTER
    float fAvg = getAvgFresnel(etaR);
    float eAvg = getAverageSSDirectionalAlbedoNoFresnel(linearRoughness);
    return fAvg * eAvg / (1.f - fAvg * (1.f - eAvg));
#else    
	return fresnelDielectricDielectric2(etaR, cosTheta);
#endif
}


//--------------------------------------Energy compensation -----------------------------------------------------//
template<typename T>
T getEnergyCompensationKulla(in T fms, in float dotWo, in float dotWi, in float linearRoughness, in T singleScattering)
{ 
	float dirAlbedoWo = getSSDirectionalAlbedoNoFresnel(abs(dotWo), linearRoughness);
	float dirAlbedoWi = getSSDirectionalAlbedoNoFresnel(abs(dotWi), linearRoughness);
	float eAvg = getAverageSSDirectionalAlbedoNoFresnel(linearRoughness);
    float ems = safeDiv((1.f - dirAlbedoWo) * (1.f - dirAlbedoWi), (1.f - eAvg) * PI);
    
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
#ifdef RECIPROCAL_MULTISCATTER
	return getEnergyCompensationKulla(fms, abs(dotWo), abs(dotWi), linearRoughness, singleScatter);
#else
	return getEnergyCompensationTurquin(fms, abs(dotWo), abs(dotWi), linearRoughness, singleScatter);
#endif
}

float getEnergyRemainingAfterSpecular(in float etaR, in float dotWo, in float dotWi, in float linearRoughness, in float specularAmount)
{
    float dirAlbedoWo = getSSMSDirectionalAlbedo(etaR, abs(dotWo), linearRoughness) * specularAmount;
    return 1.f - dirAlbedoWo;
    
}

float getEnergyRemainingAfterSheen(in float dotWo, in float dotWi, in float linearRoughness, in float sheenAmount)
{
    float dirAlbedoWo = getSheenDirectionalAlbedo(abs(dotWo), linearRoughness) * sheenAmount;
    return 1.f - dirAlbedoWo;
}

float getEnergyCompensationTranslucentKulla(in float etaR, in float dotWo, in float dotWi, in float linearRoughness, in float singleScatter)
{
    bool transmission = dotWo * dotWi < 0;
	
    float dirAlbedoWo;
    float dirAlbedoWi;
    float eAvg;
    float ratioScaling = getReflRatioScaling(etaR, linearRoughness);
    float ratio = getAvgFresnel(etaR);
	
	if(transmission)
    {
        float etaInv = 1.f / etaR;
        dirAlbedoWo = getDirectionalAlbedoTranslucent(etaR, dotWo, linearRoughness);
        dirAlbedoWi = getDirectionalAlbedoTranslucent(etaInv, dotWi, linearRoughness);
        eAvg = getAverageAlbedoTranslucent(etaInv, linearRoughness);
        ratio = 1.f - ratio;
        //ratioScaling = 1.f - ratioScaling;
        
    } 
	else
    {
        dirAlbedoWo = getDirectionalAlbedoTranslucent(etaR, dotWo, linearRoughness);
        dirAlbedoWi = getDirectionalAlbedoTranslucent(etaR, dotWi, linearRoughness);
        eAvg = getAverageAlbedoTranslucent(etaR, linearRoughness);
    }
    
    //TODO: need to apply ratioScaling factor to make sure the adjoint btdf if reciprocal. So need to pull in information about ray direction (ie. from light or from camera)
	
    float ems = ratio * safeDiv((1.f - dirAlbedoWo) * (1.f - dirAlbedoWi), (1.f - eAvg) * PI);
	return ems;
}


float getEnergyCompensationTranslucentTurquin(in float etaR, in float dotWo, in float dotWi, in float linearRoughness, in float singleScatter)
{
    float dirAlbedoWo = getDirectionalAlbedoTranslucent(etaR, abs(dotWo), linearRoughness);
    float ems = (singleScatter / dirAlbedoWo) - singleScatter;

    return ems;
}

float getEnergyCompensationTranslucent(in float etaR, in float dotWo, in float dotWi, in float linearRoughness, in float singleScatter)
{
#ifdef RECIPROCAL_MULTISCATTER
    return getEnergyCompensationTranslucentKulla(etaR, dotWo, dotWi, linearRoughness, singleScatter);
#else
    return getEnergyCompensationTranslucentTurquin(etaR, dotWo, dotWi, linearRoughness, singleScatter);
#endif

}

#endif