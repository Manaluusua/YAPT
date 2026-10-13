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
    SpectralSamples subsurfaceRadius;
	
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
	
    float thinFilmThicknessNM;
	float sheenRoughness;
	float sheenAmount;
	float emissionFocus;
    float subsurfaceAmount;
    uint occlusionSpecDiffPacked;
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
	surfDef.absorption.setFromRGBUnboundedNoIlluminant(rgb.absorption); //absorption coefficients aren't limited to [0, 1]
	surfDef.emissive.setFromRGBUnbounded(rgb.emissive);
	surfDef.sheenColor.setFromRGB(rgb.sheenColor);
	surfDef.subsurfaceRadius.setFromRGBUnboundedNoIlluminant(rgb.subsurfaceRadius); //distances aren't limited to [0, 1]

	

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
    surfDef.thinFilmThicknessNM = rgb.thinFilmThicknessNM;

	surfDef.cauchysCoeffs = rgb.cauchysCoeffs;

	surfDef.sheenRoughness = rgb.sheenRoughness;
	surfDef.sheenAmount = rgb.sheenAmount;
	surfDef.emissionFocus = rgb.emissionFocus;
	surfDef.subsurfaceAmount = rgb.subsurfaceAmount;
    surfDef.occlusionSpecDiffPacked = rgb.occlusionSpecDiffPacked;
	surfDef.flags = rgb.flags;
	return surfDef;
}

//features of the first camera vertex for the denoiser, written for every sample and accumulated like the radiance
void writeMaterialParamsForFirstBounce(uint2 outputLocation, SurfaceDefinitionRGB rgb, float3x3 normalTransform, float viewDepth, RWTexture2D<uint4> materialOutput0, RWTexture2D<uint4> materialOutput1)
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
	
    float3 normal = mul(normalTransform, rgb.baseLayerNormal);
    if (rgb.clearCoatAmount > 0)
    {
        normal += mul(normalTransform, rgb.coatingLayerNormal);
    }
	
    MaterialParameters2Texture output;
    output.normal = normalize(normal);
    //emission is compressed to [0, 1) so lights still separate from their surroundings without overflowing the half precision feature (and its variance)
    output.albedo = rgb.emissive / (1.f + rgb.emissive) + rgb.albedo + rgb.sheenColor * rgb.sheenAmount;
    output.roughness = rgb.roughness * baseWeight + rgb.clearCoatRoughness * coatingWeight + rgb.sheenRoughness * sheenWeight;
    output.ior = rgb.dielectricIOR * rgb.specularAmount + rgb.clearCoatIOR * rgb.clearCoatAmount;
    output.anisotropy = rgb.anisotropy;
    output.metalness = rgb.metalness;
    output.depth = viewDepth;
    output.transparency = rgb.transparency;
    output.materialFlags = MATERIAL_PARAMS_FLAG_HIT;
	
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
    float4 ab = exp(-absorption.toFloat4() * distance);
    s.fromFloat4(ab);
	return s;
}

// "A Fast and Robust Method for Avoiding Self-Intersection", Ray Tracing Gems, chapter 6.
float3 spawnRayOrigin(float3 position, float3 geometryNormal, float3 rayDir)
{
    const float originThreshold = 1.0f / 32.0f;
    const float floatScale = 1.0f / 65536.0f;
    const float intScale = 256.0f;

    float3 n = dot(rayDir, geometryNormal) < 0.f ? -geometryNormal : geometryNormal;
    int3 offsetInt = int3(intScale * n);
    int3 signedOffset = int3(position.x < 0 ? -offsetInt.x : offsetInt.x, position.y < 0 ? -offsetInt.y : offsetInt.y, position.z < 0 ? -offsetInt.z : offsetInt.z);
    float3 offsetPosition = asfloat(asint(position) + signedOffset);
    return float3(abs(position.x) < originThreshold ? position.x + floatScale * n.x : offsetPosition.x,
                  abs(position.y) < originThreshold ? position.y + floatScale * n.y : offsetPosition.y,
                  abs(position.z) < originThreshold ? position.z + floatScale * n.z : offsetPosition.z);
}

bool alphaTestPasses(uint instanceIndex, uint primitiveIndex, float2 barycentrics2)
{
    uint2 matMeshIndices = getMaterialAndMeshIndicesForInstance(instanceIndex);
    MaterialEntryGPU matEntry = getMaterialEntry(matMeshIndices.x);
    if (matEntry.alphaCutoff <= 0.f || matEntry.albedoTexIndexAndScale.x == TEX_UNBOUND_INDEX)
    {
        return true;
    }

    MeshEntryGPU meshEntry = getMeshEntry(matMeshIndices.y);
    if (!meshHasValidUVs(meshEntry.uvBuffer))
    {
        return true;
    }

    float3 barycentrics = float3(1 - barycentrics2.x - barycentrics2.y, barycentrics2.x, barycentrics2.y);
    uint3 indices = fetchIndices(meshEntry.indexBuffer, primitiveIndex);
    float2 uv = fetchMeshUV(meshEntry.uvBuffer, indices, barycentrics);
    float2 uvScale = unpackTextureTransformScale(matEntry.albedoTexIndexAndScale.y);
    float alpha = g_textures2D[matEntry.albedoTexIndexAndScale.x].SampleLevel(g_colorSampler, uv * uvScale, 0).a;
    return alpha >= matEntry.alphaCutoff;
}

#define PROCEED_ALPHA_TESTED(q)     while (q.Proceed())     {         if (q.CandidateType() == CANDIDATE_NON_OPAQUE_TRIANGLE && alphaTestPasses(q.CandidateInstanceID(), q.CandidatePrimitiveIndex(), q.CandidateTriangleBarycentrics()))         {             q.CommitNonOpaqueTriangleHit();         }     }

void fillSurfaceDefRGB(in uint instanceIndex, in uint primitiveIndex, in float2 barycentrics2, out SurfaceDefinitionRGB surfaceDefOut)
{
    uint2 matMeshIndices = getMaterialAndMeshIndicesForInstance(instanceIndex);
    MeshEntryGPU meshEntry = getMeshEntry(matMeshIndices.y);

	//Initial surface setup
    float3 barycentrics = float3(1 - barycentrics2.x - barycentrics2.y, barycentrics2.x, barycentrics2.y);
    uint3 indices = fetchIndices(meshEntry.indexBuffer, primitiveIndex);
    float3 geometryNormal = fetchMeshTriangleNormal(meshEntry.positionBuffer, indices, barycentrics);
    float3 normal = fetchMeshNormal(meshEntry.normalBuffer, indices, barycentrics);
    float3 tangent = meshHasValidTangents(meshEntry.tangentBuffer) ? fetchMeshTangent(meshEntry.tangentBuffer, indices, barycentrics) : float3(1.f, 0.f, 0.f);
    float2 uv = meshHasValidUVs(meshEntry.uvBuffer) ? fetchMeshUV(meshEntry.uvBuffer, indices, barycentrics) : float2(0.5f, 0.5f);


	//fetch surface material parameters
    MaterialEntryGPU matEntry = getMaterialEntry(matMeshIndices.x);
    SurfaceDefinitionRGB surfaceDefRGB;
    fetchSurfaceMaterialParameters(matEntry, surfaceDefRGB, geometryNormal, normal, normal, tangent);
    setupSurfaceOrientation(surfaceDefRGB);
    modifySurfaceMaterialParametersWithTextures(matEntry, uv, normal, tangent, surfaceDefRGB);
    
    
    surfaceDefOut = surfaceDefRGB;
}

template<typename RayState>
void handleRayStartInsideMedium(inout RayState state, in SurfaceDefinition surfaceDef, uint materialIndex, float3 rayDir, float3 geometryNormal)
{
    
    //special case where the ray starts from inside a medium:
    if (dot(rayDir, geometryNormal) > 0 && state.getNumberOfVolumesEntered() == 0 && !isSurfaceTwoSided(surfaceDef.flags))
    {
        state.enteredVolume(surfaceDef.dielectricIOR, surfaceDef.absorption, materialIndex);
    }

}



#endif