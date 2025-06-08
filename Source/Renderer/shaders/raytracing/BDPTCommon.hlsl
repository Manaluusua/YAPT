#ifndef BDPT_COMMON_HLSL
#define BDPT_COMMON_HLSL

#include "../globalDefinitions.hlsl"
#include "materialSample.hlsl"
#include "materialModifiers.hlsl"

struct BidirectionalPathTraceConstants
{
	float4 worldBoundsMin;
	float4 worldBoundsMax;
	uint envTextureIndex;
	uint envType;
};

struct RenderObjectEntry
{
	uint2 meshAndMaterialIndices;
};


struct LightEntryGPU
{
	float4x4 transform;
	uint meshIndex;
	uint matIndex;
};
struct LightPathNode
{
	float4 normalPDF;
	float4 positionDummy;
};

ConstantBuffer<BidirectionalPathTraceConstants> g_bdptConstants : register(b0, space3);
StructuredBuffer<RenderObjectEntry> g_renderObjects : register(t1, space3);
StructuredBuffer<LightEntryGPU> g_lights : register(t2, space3);
RaytracingAccelerationStructure g_accelerationStructure : register(t3, space3);
RWTexture2D<float4> g_outputColor : register(u4, space3);

RenderObjectEntry getRenderObject(uint index)
{
	return g_renderObjects[index];
}

float3 generateRayDirection(float2 uv)
{

	float4 pointOnNearPlane = float4(uv, 0.0f, 1.0f);

	pointOnNearPlane = mul(g_uvToViewTransform, pointOnNearPlane);
	pointOnNearPlane /= pointOnNearPlane.w;
	pointOnNearPlane.w = 0;

	float3 worldDir = mul(g_viewToWorldTransform, pointOnNearPlane).xyz;

	float3 rayDir = normalize(worldDir);
	return rayDir;
}


#endif