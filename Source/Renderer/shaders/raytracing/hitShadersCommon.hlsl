#ifndef HITSHADERSCOMMON_HLSL_INCL
#define HITSHADERSCOMMON_HLSL_INCL

#include "../materials/materialsCommon.hlsl"
#include "raytraceCommonResources.hlsl"
#include "../postprocess/materialParameterTextures.hlsl"

#define MaterialMask_TwoSided (1 << 0)
#define MaterialMask_Dispersion (1 << 1)
#define TEX_UNBOUND_INDEX (~0)


struct SurfaceDefinition
{
    SpectralSamples specular;
    SpectralSamples albedo;
    SpectralSamples absorption;
    SpectralSamples emissive;
    SpectralSamples sheenColor;
	
    float3 coatingLayerNormal;
    float transparency;
    float3 baseLayerNormal;
    float metalness;
    float3 geometryNormal;
    float dielectricIOR;
    float3 tangent;
    float roughness;

    float2 cauchysCoeffs;
	float specularAmount;
	float clearCoatAmount;
	
	float clearCoatIOR;
	float clearCoatRoughness;
	float anisotropy;
	float anisotropyRotation;
	
	float thinFilmThickness;
	float sheenRoughness;
	float sheenAmount;
	uint flags;
};



SurfaceDefinition convertSurfaceDefinitionFromRGB(SurfaceDefinitionRGB rgb)
{
	SurfaceDefinition surfDef;
    surfDef.coatingLayerNormal = rgb.coatingLayerNormal;
    surfDef.baseLayerNormal = rgb.baseLayerNormal;
    surfDef.geometryNormal = rgb.geometryNormal;
    surfDef.tangent = rgb.tangent;

	surfDef.specular.setFromRGB(rgb.specular);
	surfDef.albedo.setFromRGB(rgb.albedo);
	surfDef.absorption.setFromRGB(rgb.absorption);
	surfDef.emissive.setFromRGBUnbounded(rgb.emissive);
	surfDef.sheenColor.setFromRGB(rgb.sheenColor);

	

	surfDef.transparency = rgb.transparency;
	surfDef.metalness = rgb.metalness;
	surfDef.dielectricIOR = rgb.dielectricIOR;
	surfDef.roughness = rgb.roughness;
	surfDef.specularAmount = rgb.specularAmount;

	surfDef.clearCoatAmount = rgb.clearCoatAmount;
	surfDef.clearCoatIOR = rgb.clearCoatIOR;
	surfDef.clearCoatRoughness = rgb.clearCoatRoughness;

	surfDef.anisotropy = rgb.anisotropy;
	surfDef.anisotropyRotation = rgb.anisotropyRotation;
	surfDef.thinFilmThickness = rgb.thinFilmThickness;

	surfDef.cauchysCoeffs = rgb.cauchysCoeffs;

	surfDef.sheenRoughness = rgb.sheenRoughness;
	surfDef.sheenAmount = rgb.sheenAmount;
	surfDef.flags = rgb.flags;
	return surfDef;
}

void writeMaterialParamsForFirstBounce(uint2 outputLocation, SurfaceDefinitionRGB rgb, float depth, RWTexture2D<uint4> materialOutput0, RWTexture2D<uint4> materialOutput1)
{
    float coatingWeight = rgb.clearCoatAmount;
    float sheenWeight = rgb.sheenAmount;
    float baseWeight = rgb.specularAmount;
	
    float weightSum = coatingWeight + sheenWeight + baseWeight;
	if(weightSum == 0.f)
    {
        weightSum = 1;
    }
    coatingWeight /= weightSum;
    sheenWeight /= weightSum;
    baseWeight /= weightSum;
	
    MaterialParameters2Texture output;
    output.normal = normalize(rgb.baseLayerNormal + rgb.coatingLayerNormal * rgb.clearCoatAmount);
    output.albedo = rgb.emissive + rgb.albedo + rgb.sheenColor * rgb.sheenAmount;
    output.roughness = rgb.roughness * baseWeight + rgb.clearCoatRoughness * coatingWeight + rgb.sheenRoughness * sheenWeight;
    output.ior = rgb.dielectricIOR * rgb.specularAmount + rgb.clearCoatIOR * rgb.clearCoatAmount;
    output.anisotropy = rgb.anisotropy;
    output.metalness = rgb.metalness;
    output.depth = depth;
    output.transparency = rgb.transparency;
    output.materialFlags = 0;
	
    writeMaterialParametersToTextures(outputLocation, output, materialOutput0, materialOutput1);

}

void writeEmptyMaterialParamsForFirstBounce(uint2 outputLocation, RWTexture2D<uint4> materialOutput0, RWTexture2D<uint4> materialOutput1)
{
    MaterialParameters2Texture output = (MaterialParameters2Texture) 0;
	
    writeMaterialParametersToTextures(outputLocation, output, materialOutput0, materialOutput1);
}

SpectralSamples calculateTransmittance(float distance, SpectralSamples absorption)
{
	SpectralSamples s;
	for (uint i = 0; i < absorption.getSampleCount(); ++i)
	{
		float v = exp(-absorption[i] * distance);
		s.setInd(i, v);
	}
	return s;
}

float3 getRaySpawnOffsetUsingNormal(float3 geometryNormal, float3 nextSampleDir)
{
    float offsetEpsilon = 0.001f;
    bool transmitted = dot(nextSampleDir, geometryNormal) < 0.f ? true : false;
		
    float3 rayOffset = geometryNormal * offsetEpsilon;
    rayOffset *= transmitted ? -1.f : 1.f;
    return rayOffset;
}

float3 getRaySpawnOffsetTowardsRay(float3 nextSampleDir)
{
    float offsetEpsilon = 0.001f;
    return nextSampleDir * offsetEpsilon;
}

#endif