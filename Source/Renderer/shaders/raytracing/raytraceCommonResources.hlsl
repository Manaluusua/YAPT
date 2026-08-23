#ifndef RAYTRACE_COMMON_RESOURCES_HLSL_INCL
#define RAYTRACE_COMMON_RESOURCES_HLSL_INCL

#include "raytraceCommon.hlsl"
#include "../materials/materialsCommonResources.hlsl"
//data structs, keep in sync with raytracestage

struct RaytraceConstantData
{
    float4x4 uvToView;
    float4x4 viewToUV;
    float4x4 worldToView;
    float4x4 viewToWorld;
    float4 worldBoundsMin;
    float4 worldBoundsMax;
	float4 cameraPosition;
	float4 targetTexDimensions;
    float4 targetOffsetScaleBias;
	float2 rayUVOffset;
	uint maxRayDepth;
    uint envTextureIndex;
    uint envType;
    float envIntensityScale;
    uint lightCount;
    uint flags;
    uint regularizeAfterVertices;
    uint pad0;
    uint pad1;
    uint pad2;
};

//uniforms
ConstantBuffer<RaytraceConstantData> g_rayGenConstants : register(b1, space0);
ByteAddressBuffer g_randomSampleLocations : register(t2, space0);
StructuredBuffer<RenderObjectTransformDataGPU> g_renderObjectTransforms : register(t3, space0);
ByteAddressBuffer g_renderObjectMatAndMeshIndices : register(t4, space0);
StructuredBuffer<LightEntryGPU> g_lights : register(t5, space0);

#define CONSTANTS_FLAG_WRITE_FIRST_BOUNCE_MATERIAL_PARAMS  (1 << 0)
#define CONSTANTS_FLAG_DISABLE_TEXEL_JITTER  (1 << 1)

//helper defines
#define g_uvToViewTransform g_rayGenConstants.uvToView
#define g_viewToWorldTransform g_rayGenConstants.viewToWorld
#define g_viewToUVTransform g_rayGenConstants.viewToUV
#define g_worldToViewTransform g_rayGenConstants.worldToView
#define g_constantsFlags g_rayGenConstants.flags
#define g_cameraPosition g_rayGenConstants.cameraPosition.xyz
#define g_rayDirUvOffset g_rayGenConstants.rayUVOffset.xy
#define g_maxRayDepth g_rayGenConstants.maxRayDepth
#define g_regularizeAfterVertices g_rayGenConstants.regularizeAfterVertices
#define g_targetTexDimensions g_rayGenConstants.targetTexDimensions
#define g_targetOffsetScaleBias g_rayGenConstants.targetOffsetScaleBias
#define g_lightCount g_rayGenConstants.lightCount
#define g_worldBoundsMin g_rayGenConstants.worldBoundsMin.xyz
#define g_worldBoundsMax g_rayGenConstants.worldBoundsMax.xyz
#define g_envType g_rayGenConstants.envType
#define g_envTexIndex g_rayGenConstants.envTextureIndex
#define g_envIntensityScale g_rayGenConstants.envIntensityScale

#define g_randomSamples g_randomSampleLocations

bool isPixelJitterEnabled()
{
    return (g_constantsFlags & CONSTANTS_FLAG_DISABLE_TEXEL_JITTER) == 0;

}

bool firstBounceMaterialWriteEnabled()
{
    return (g_constantsFlags & CONSTANTS_FLAG_WRITE_FIRST_BOUNCE_MATERIAL_PARAMS) != 0;

}

uint2 dispatchIndicesToRayIndices(uint2 dispatchInd)
{
    return dispatchInd.xy * g_targetOffsetScaleBias.xy + g_targetOffsetScaleBias.zw;

}

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

float3 generateRayDirection(float2 uv, float2 rand)
{
    float2 texelOffset;
    if (isPixelJitterEnabled())
    {
        texelOffset = rand.xy - 0.5f; //[-0.5, 0.5], assuming the uv is at texel center
    } 
    else
    {
        texelOffset = 0;
    }
    texelOffset *= g_targetTexDimensions.zw;
    
    float4 pointOnNearPlane = float4(uv + texelOffset, 0.0f, 1.0f);
	
    pointOnNearPlane = mul(g_uvToViewTransform, pointOnNearPlane);
    pointOnNearPlane /= pointOnNearPlane.w;
    pointOnNearPlane.w = 0;
	
    float3 worldDir = mul(g_viewToWorldTransform, pointOnNearPlane).xyz;
	
    float3 rayDir = normalize(worldDir);
    return rayDir;
}

#define GET_RANDOM_NUMBER_SEQUENCE_1(dimOffset) (g_randomSamples.Load(dimOffset << 2))
#define GET_RANDOM_NUMBER_SEQUENCE_2(dimOffset) (g_randomSamples.Load2(dimOffset << 2))
#define GET_RANDOM_NUMBER_SEQUENCE_3(dimOffset) (g_randomSamples.Load3(dimOffset << 2))
#define GET_RANDOM_NUMBER_SEQUENCE_4(dimOffset) (g_randomSamples.Load4(dimOffset << 2))

#include "../common/randomSampler.hlsl"
#include "spectralDistribution.hlsl"
#include "rayState.hlsl"

#endif
