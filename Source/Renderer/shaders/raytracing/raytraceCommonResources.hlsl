#ifndef RAYTRACE_COMMON_RESOURCES_HLSL_INCL
#define RAYTRACE_COMMON_RESOURCES_HLSL_INCL

#include "raytraceCommon.hlsl"
#include "../materials/materialsCommonResources.hlsl"
//data structs, keep in sync with raytracestage

struct RaytraceConstantData
{
	float4x4 uvToView;
	float4x4 viewToWorld;
    float4 worldBoundsMin;
    float4 worldBoundsMax;
	float4 cameraPosition;
	float4 targetTexDimensions;
	float2 rayUVOffset;
	uint maxRayDepth;
    uint envTextureIndex;
    uint envType;
    uint lightCount;
};

//uniforms
ConstantBuffer<RaytraceConstantData> g_rayGenConstants : register(b1, space0);
ByteAddressBuffer g_randomSampleLocations : register(t2, space0);
StructuredBuffer<RenderObjectTransformDataGPU> g_renderObjectTransforms : register(t3, space0);
ByteAddressBuffer g_renderObjectMatAndMeshIndices : register(t4, space0);
StructuredBuffer<LightEntryGPU> g_lights : register(t5, space0);

//helper defines
#define g_uvToViewTransform g_rayGenConstants.uvToView
#define g_viewToWorldTransform g_rayGenConstants.viewToWorld
#define g_cameraPosition g_rayGenConstants.cameraPosition.xyz
#define g_rayDirUvOffset g_rayGenConstants.rayUVOffset.xy
#define g_maxRayDepth g_rayGenConstants.maxRayDepth
#define g_targetTexDimensions g_rayGenConstants.targetTexDimensions
#define g_lightCount g_rayGenConstants.lightCount
#define g_worldBoundsMin g_rayGenConstants.worldBoundsMin.xyz
#define g_worldBoundsMax g_rayGenConstants.worldBoundsMax.xyz
#define g_envType g_rayGenConstants.envType
#define g_envTexIndex g_rayGenConstants.envTextureIndex

#define g_randomSamples g_randomSampleLocations


uint2 getMaterialAndMeshIndicesForInstance(uint instanceIndex)
{
    uint readOffset = instanceIndex << 3;
    uint2 matMeshIndices = g_renderObjectMatAndMeshIndices.Load2(readOffset);
    return matMeshIndices;
}

RenderObjectTransformDataGPU getTransformDataForInstance(uint instanceIndex)
{
    return g_renderObjectTransforms[instanceIndex];

}

float4 getWorldCenterAndRadiusSqr()
{
    float3 c = (g_worldBoundsMax + g_worldBoundsMin) * 0.5f;
    float3 ext = (g_worldBoundsMax - c);
    return float4(c, dot(ext, ext));
}

//helper functions

uint scramble(uint random, uint scrambleSeed, uint dimension)
{
    uint h = hash(scrambleSeed, dimension);
    return owenScrambleBase2(random, h);
}

float getRandomSampleFloat(uint dimensionSetIndex, uint seed)
{
    uint startIndex = dimensionSetIndex << 2;
    uint sobolSeq = g_randomSamples.Load(startIndex);
    return uintToFloat01(scramble(sobolSeq, seed, dimensionSetIndex));
}

float2 getRandomSampleFloat2(uint dimensionSetIndex, uint seed)
{
    uint startIndex = dimensionSetIndex << 2;
    uint2 sobolSeq = g_randomSamples.Load2(startIndex);
    sobolSeq.xy = uint2(scramble(sobolSeq.x, seed, dimensionSetIndex), scramble(sobolSeq.y, seed, dimensionSetIndex + 1));
    return uintToFloat01(sobolSeq);
}

float3 getRandomSampleFloat3(uint dimensionSetIndex, uint seed)
{
    uint startIndex = dimensionSetIndex << 2;
    uint3 sobolSeq = g_randomSamples.Load3(startIndex);
    sobolSeq.xyz = uint3(scramble(sobolSeq.x, seed, dimensionSetIndex), scramble(sobolSeq.y, seed, dimensionSetIndex + 1), scramble(sobolSeq.z, seed, dimensionSetIndex + 2));
    return uintToFloat01(sobolSeq);
}

float4 getRandomSampleFloat4(uint dimensionSetIndex, uint seed)
{
    uint startIndex = dimensionSetIndex << 2;
    uint4 sobolSeq = g_randomSamples.Load4(startIndex);
    sobolSeq.xyzw = uint4(scramble(sobolSeq.x, seed, dimensionSetIndex), scramble(sobolSeq.y, seed, dimensionSetIndex + 1), scramble(sobolSeq.z, seed, dimensionSetIndex + 2), scramble(sobolSeq.w, seed, dimensionSetIndex + 3));
    return uintToFloat01(sobolSeq);

}

struct RandomSampler
{
    uint2 dimensionOffsetAndSeed;
	
	void init(uint dim, uint pixelIndex)
	{
        dimensionOffsetAndSeed.x = dim;
        dimensionOffsetAndSeed.y = pixelIndex;
    }
		
    float getRandom1()
    {
        float v = getRandomSampleFloat(dimensionOffsetAndSeed.x, dimensionOffsetAndSeed.y);
        ++dimensionOffsetAndSeed.x;
        return v;
    }
	
    float2 getRandom2()
    {
        float2 v = getRandomSampleFloat2(dimensionOffsetAndSeed.x, dimensionOffsetAndSeed.y);
        dimensionOffsetAndSeed.x += 2;
        return v;
    }
    float3 getRandom3()
    {
        float3 v = getRandomSampleFloat3(dimensionOffsetAndSeed.x, dimensionOffsetAndSeed.y);
        dimensionOffsetAndSeed.x += 3;
		return v;
    }
	
    float4 getRandom4()
    {

        float4 v = getRandomSampleFloat4(dimensionOffsetAndSeed.x, dimensionOffsetAndSeed.y);
        dimensionOffsetAndSeed.x += 4;
		return v;
    }
	
    
};


#include "spectralDistribution.hlsl"
#include "rayState.hlsl"


#endif
