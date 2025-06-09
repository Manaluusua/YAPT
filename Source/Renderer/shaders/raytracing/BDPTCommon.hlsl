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
	float4x4 transformInvTransp;
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

#define g_sampledWavelengths g_spectralSamplingConstants.spdSampleLambda
#define g_sampledWavelengthPDFs g_spectralSamplingConstants.spdSamplePdf

#define g_worldBoundsMin g_bdptConstants.worldBoundsMin.xyz
#define g_worldBoundsMax g_bdptConstants.worldBoundsMax.xyz

float4 getWorldCenterAndRadiusSqr()
{
	float3 c = (g_worldBoundsMax + g_worldBoundsMin) * 0.5f;
	float3 ext = (g_worldBoundsMax - c);
	return float4(c, dot(ext, ext));
}

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


void sampleEnvironmentLighting(float4 randValues, out float3 posOut, out float3 dirOut, out float pdfOut)
{
	//sample direction
	float pdfDir;
	float3 lightDir;
	{
		float2 uv = randValues.xy;
		float theta = uv.x * PI;
		float phi = uv.y * 2 * PI;
		float cosTheta;
		float sinTheta;
		float cosPhi;
		float sinPhi;

		sincos(theta, sinTheta, cosTheta);
		sincos(phi, sinPhi, cosPhi);

		lightDir = float3(sinTheta * cosPhi, sinTheta * sinPhi, cosTheta);
		pdfDir = safeDiv(1.f, (2.f * PI * PI * sinTheta));
	}

	//sample position
	float pdfPos;
	float3 lightPos;
	{
		float4 worldCenterRad = getWorldCenterAndRadiusSqr();
		float3 v1, v2;
		constructVectorBase(lightDir, v1, v2);
		float2 cd = sampleConcentricDisk(randValues.zw);
		lightPos = worldCenterRad.xyz + sqrt(worldCenterRad.w) * (cd.x * v1 + cd.y * v2);
		pdfPos = 1 / (PI * worldCenterRad.w);
	}

	posOut = lightPos;
	dirOut = lightDir;
	pdfOut = pdfDir * pdfPos;
	
}

float convertSolidAngleToSurfaceArea(float solidAnglePDF, float3 fromToUnnormalized, float3 toNormal)  
{
	float invDistSqr = 1.f/dot(fromToUnnormalized, fromToUnnormalized);
	float pdf = solidAnglePDF * abs(dot(toNormal, fromToUnnormalized * sqrt(invDistSqr)));
	return pdf * invDistSqr;
}

#endif