#ifndef BDPT_COMMON_HLSL
#define BDPT_COMMON_HLSL

#include "../globalDefinitions.hlsl"
#include "materialSample.hlsl"
#include "materialModifiers.hlsl"

struct BidirectionalPathTraceConstants
{
	float4 worldBoundsMin;
	float4 worldBoundsMax;
	uint2 lightPathsPerDim;
	uint envTextureIndex;
	uint envType;
	uint maxVerticesPerLightPath;
	uint maxAllocatedVertices;
	uint lightCount;
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

struct LightPathHeader
{
	uint2 offsetAndCount;
};

struct LightPathNode
{
	float4 normalPDF;
	float4 positionMISSum;
};

ConstantBuffer<BidirectionalPathTraceConstants> g_bdptConstants : register(b0, space3);
StructuredBuffer<RenderObjectEntry> g_renderObjects : register(t1, space3);
StructuredBuffer<LightEntryGPU> g_lights : register(t2, space3);
RaytracingAccelerationStructure g_accelerationStructure : register(t3, space3);

#ifdef WRITABLE_LIGHT_DATA
RWStructuredBuffer<LightPathHeader> g_lightPathHeaders : register(u4, space3);
RWStructuredBuffer<LightPathNode> g_lightPathVertices : register(u5, space3);
RWByteAddressBuffer g_counters : register(u6, space3);
#else
StructuredBuffer<LightPathHeader> g_lightPathHeaders : register(t4, space3);
StructuredBuffer<LightPathNode> g_lightPathVertices : register(t5, space3);
ByteAddressBuffer g_counters : register(t6, space3);
#endif

#define COUNTER_LIGHT_HEADERS_INDEX 0
#define COUNTER_LIGHT_VERTICES_INDEX 1

#define g_worldBoundsMin g_bdptConstants.worldBoundsMin.xyz
#define g_worldBoundsMax g_bdptConstants.worldBoundsMax.xyz
#define g_maxVerticesPerLightPath g_bdptConstants.maxVerticesPerLightPath
#define g_maxAllocatedVertices g_bdptConstants.maxAllocatedVertices
#define g_lightPathsPerDim g_bdptConstants.lightPathsPerDim
#define g_lightCount g_bdptConstants.lightCount

#define g_envType g_bdptConstants.envType
#define g_envTexIndex g_bdptConstants.envTextureIndex

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


void sampleEnvironmentLighting(float4 randValues, out SpectralSamples radianceOut, out float3 posOut, out float3 dirOut, out float pdfOut)
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

	radianceOut.setFromRGBUnbounded(getSkyBoxColor(-lightDir, g_envType, g_envTexIndex).xyz);
	posOut = lightPos;
	dirOut = lightDir;
	pdfOut = pdfDir * pdfPos;
	
}

void sampleLight(uint lightIndex, float4 randValues, out SpectralSamples radianceOut, out float3 posOut, out float3 dirOut, out float pdfOut)
{
	LightEntryGPU lightEntry = g_lights[lightIndex];
	MeshEntryGPU meshEntry = getMeshEntry(lightEntry.meshIndex);

	uint primCount = (meshEntry.indexCount / 3);
	uint primitiveIndex = min(primCount * randValues.z, primCount - 1);

	float3 barycentrics = float3(1 - randValues.x - randValues.y, randValues.x, randValues.y);
	uint3 indices = fetchIndices(meshEntry.indexBuffer, primitiveIndex);

	float3 p1, p2, p3;
	fetchMeshPositions(meshEntry.positionBuffer, indices, p1, p2, p3);
	float area = length(cross(p2 - p1, p3 - p1)) * 0.5f;
	float3 pos = barycentrics.x * p1 + barycentrics.y * p2 + barycentrics.z * p3;

	float3 normal = fetchMeshNormal(meshEntry.normalBuffer, indices, barycentrics);
	float3 tangent = meshHasValidTangents(meshEntry.tangentBuffer) ? fetchMeshTangent(meshEntry.tangentBuffer, indices, barycentrics) : float3(1.f, 0.f, 0.f);
	float2 uv = meshHasValidUVs(meshEntry.uvBuffer) ? fetchMeshUV(meshEntry.uvBuffer, indices, barycentrics) : float2(0.5f, 0.5f);


	MaterialEntryGPU matEntry = getMaterialEntry(lightEntry.matIndex);
	SurfaceDefinitionRGB surfaceDefRGB;
	fetchSurfaceMaterialParameters(matEntry, surfaceDefRGB);

	modifySurfaceMaterialParametersWithTextures(matEntry, uv, normal, tangent, surfaceDefRGB);


	radianceOut.setFromRGBUnbounded(surfaceDefRGB.emissive);
	pdfOut = 1.f / (primCount * area);
	posOut = pos;
	dirOut = normal; 

}

float convertSolidAngleToSurfaceArea(float solidAnglePDF, float3 fromToUnnormalized, float3 toNormal)  
{
	float invDistSqr = 1.f/dot(fromToUnnormalized, fromToUnnormalized);
	float pdf = solidAnglePDF * abs(dot(toNormal, fromToUnnormalized * sqrt(invDistSqr)));
	return pdf * invDistSqr;
}


void sampleLight(float lightPickRand, float4 lightSampleRand, float envSampleRelativeProbability, out SpectralSamples radianceOut, out float3 posOut, out float3 dirOut, out float pdfOut)
{
	if (g_lightCount == 0)
	{
		sampleEnvironmentLighting(lightSampleRand, radianceOut, posOut, dirOut, pdfOut);
	}

	float lightProb = g_lightCount + envSampleRelativeProbability;
	float s = lightPickRand * lightProb;
	if (s > g_lightCount)
	{
		sampleEnvironmentLighting(lightSampleRand, radianceOut, posOut, dirOut, pdfOut);
	}
	else
	{
		uint lightIndex = min((uint)floor(lightPickRand * g_lightCount), g_lightCount - 1);
		sampleLight(lightIndex, lightSampleRand, radianceOut,  posOut, dirOut, pdfOut);
	}

}

#endif