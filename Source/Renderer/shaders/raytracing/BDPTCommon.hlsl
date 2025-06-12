#ifndef BDPT_COMMON_HLSL
#define BDPT_COMMON_HLSL

#include "../globalDefinitions.hlsl"
#include "materialSample.hlsl"
#include "rayState.hlsl"

struct BidirectionalPathTraceConstants
{
	float4 worldBoundsMin;
	float4 worldBoundsMax;
	uint2 lightPathsPerDim;
	uint envTextureIndex;
	uint envType;
	uint maxVerticesPerLightPath;
	uint maxAllocatedVertices;
};


struct LightPathHeader
{
	uint2 offsetAndCount;
};

struct LightPathNode
{
    float4 throughput;
    float4 normalWSPDF;
    float4 positionWSMISSum;
    float4 instancePrimitiveBarycentrics;
};

struct BDPTRayState //: RayStateInterface
{

    float getCurrentIOR()
    {
        float currentIOR = IOR_DEFAULT; //air if not entered volume
        if (numberVolumesEntered != 0)
        {
            currentIOR = volumesEntered[numberVolumesEntered - 1];
        }
        return currentIOR;
    }
    float getPreviousIOR()
    {
        float beforeCurrentIOR = IOR_DEFAULT;
        if (numberVolumesEntered > 1)
        {
            beforeCurrentIOR = volumesEntered[numberVolumesEntered - 2];
        }
        return beforeCurrentIOR;
    }

    void enteredVolume(float IOR, SpectralSamples absorptionParam)
    {
        numberVolumesEntered = min(numberVolumesEntered + 1, RAY_MAX_VOLUMES_ENTERED);
        volumesEntered[numberVolumesEntered - 1] = IOR;
        absorption = absorptionParam;
    }
    void exitedVolume()
    {
        numberVolumesEntered = max(0, numberVolumesEntered - 1);
    }

    uint getStateFlags()
    {
        return flags;
    }
    void setStateFlags(uint flagsIn)
    {
        flags = flagsIn;
    }

    void addFlags(uint flags)
    {
        setStateFlags(getStateFlags() | flags);
    }

    SpectralSamples absorption;
    float volumesEntered[RAY_MAX_VOLUMES_ENTERED];
    uint numberVolumesEntered;
    uint flags;
};



ConstantBuffer<BidirectionalPathTraceConstants> g_bdptConstants : register(b0, space3);

RaytracingAccelerationStructure g_accelerationStructure : register(t1, space3);

#ifdef WRITABLE_LIGHT_DATA
RWStructuredBuffer<LightPathHeader> g_lightPathHeaders : register(u2, space3);
RWStructuredBuffer<LightPathNode> g_lightPathVertices : register(u3, space3);
RWByteAddressBuffer g_counters : register(u4, space3);
#else
StructuredBuffer<LightPathHeader> g_lightPathHeaders : register(t2, space3);
StructuredBuffer<LightPathNode> g_lightPathVertices : register(t3, space3);
ByteAddressBuffer g_counters : register(t4, space3);
#endif

#define COUNTER_LIGHT_HEADERS_INDEX 0
#define COUNTER_LIGHT_VERTICES_INDEX 1

#define g_worldBoundsMin g_bdptConstants.worldBoundsMin.xyz
#define g_worldBoundsMax g_bdptConstants.worldBoundsMax.xyz
#define g_maxVerticesPerLightPath g_bdptConstants.maxVerticesPerLightPath
#define g_maxAllocatedVertices g_bdptConstants.maxAllocatedVertices
#define g_lightPathsPerDim g_bdptConstants.lightPathsPerDim


#define g_envType g_bdptConstants.envType
#define g_envTexIndex g_bdptConstants.envTextureIndex

float4 getWorldCenterAndRadiusSqr()
{
	float3 c = (g_worldBoundsMax + g_worldBoundsMin) * 0.5f;
	float3 ext = (g_worldBoundsMax - c);
	return float4(c, dot(ext, ext));
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

void evaluateSurfaceAndPDFs(in SurfaceDefinition surfaceDef, in PrecalculatedSurfaceData precalculatedSurfaceData, in float samplingProbabilities[LAYER_COUNT], inout BDPTRayState rayState, in float3 woOS, in float3 wiOS, bool triangleHitFrontFace,
out SpectralSamples weightOut, out float pdfForward, out float pdfBackward)
{

    SpectralSamples weightDummy;
    BDPTRayState rayStateDummy = rayState;
			
    evaluateSurface(surfaceDef, woOS, wiOS, samplingProbabilities, precalculatedSurfaceData, false, rayState, weightOut, pdfForward);
    evaluateSurface(surfaceDef, wiOS, woOS, samplingProbabilities, precalculatedSurfaceData, true, rayStateDummy, weightDummy, pdfBackward); //generate pdf for reversed order
}

void calculateCommonSurfaceParams(in BDPTRayState rayState, in uint instanceIndex, in uint primitiveIndex, in float2 barycentrics2, in float3 woOS, bool triangleHitFrontFace, in float2 materialLayerRands,
out SurfaceDefinition surfaceDef, out PrecalculatedSurfaceData precalculatedSurfaceData, out float samplingProbabilities[LAYER_COUNT])
{
    RenderObjectEntry ro = g_renderObjects[instanceIndex];
    MeshEntryGPU meshEntry = getMeshEntry(ro.meshAndMaterialIndices.x);

	//Initial surface setup
    float3 barycentrics = float3(1 - barycentrics2.x - barycentrics2.y, barycentrics2.x, barycentrics2.y);
    uint3 indices = fetchIndices(meshEntry.indexBuffer, primitiveIndex);
    float3 geometryNormal = fetchMeshNormal(meshEntry.normalBuffer, indices, barycentrics);
    float3 tangent = meshHasValidTangents(meshEntry.tangentBuffer) ? fetchMeshTangent(meshEntry.tangentBuffer, indices, barycentrics) : float3(1.f, 0.f, 0.f);
    float2 uv = meshHasValidUVs(meshEntry.uvBuffer) ? fetchMeshUV(meshEntry.uvBuffer, indices, barycentrics) : float2(0.5f, 0.5f);

    float3 normal = geometryNormal;

	//fetch surface material parameters
    MaterialEntryGPU matEntry = getMaterialEntry(ro.meshAndMaterialIndices.y);
    SurfaceDefinitionRGB surfaceDefRGB;
    fetchSurfaceMaterialParameters(matEntry, surfaceDefRGB);
    modifySurfaceMaterialParametersWithTextures(matEntry, uv, normal, tangent, surfaceDefRGB);
    surfaceDef = convertSurfaceDefinitionFromRGB(surfaceDefRGB);

    getPrecalculatedSurfaceData(surfaceDef, rayState, woOS, triangleHitFrontFace, precalculatedSurfaceData);
    calculateNormalizedMaterialLayerSamplingProbabilities(surfaceDef, precalculatedSurfaceData, materialLayerRands, samplingProbabilities);
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

	float3 normal = fetchMeshNormal(meshEntry.normalBuffer, indices, barycentrics);
	float3 tangent = meshHasValidTangents(meshEntry.tangentBuffer) ? fetchMeshTangent(meshEntry.tangentBuffer, indices, barycentrics) : float3(1.f, 0.f, 0.f);
	float2 uv = meshHasValidUVs(meshEntry.uvBuffer) ? fetchMeshUV(meshEntry.uvBuffer, indices, barycentrics) : float2(0.5f, 0.5f);

	MaterialEntryGPU matEntry = getMaterialEntry(lightEntry.matIndex);
	SurfaceDefinitionRGB surfaceDefRGB;
	fetchSurfaceMaterialParameters(matEntry, surfaceDefRGB);
	modifySurfaceMaterialParametersWithTextures(matEntry, uv, normal, tangent, surfaceDefRGB);

	normal = mul(lightEntry.transformInvTransp, float4(normal, 1.f)).xyz;

	float3 p1, p2, p3;
	fetchMeshPositions(meshEntry.positionBuffer, indices, p1, p2, p3);
	p1 = mul(lightEntry.transform, float4(p1, 1)).xyz;
	p2 = mul(lightEntry.transform, float4(p2, 1)).xyz;
	p3 = mul(lightEntry.transform, float4(p3, 1)).xyz;
	float area = length(cross(p2 - p1, p3 - p1)) * 0.5f;
	float3 pos = barycentrics.x * p1 + barycentrics.y * p2 + barycentrics.z * p3;

	radianceOut.setFromRGBUnbounded(surfaceDefRGB.emissive);
	pdfOut = 1.f / (primCount * area);
	posOut = pos;
	dirOut = normal; 

}

float areaDensityMultiplier(float3 fromToUnnormalized, float3 toNormal)  
{
	float invDistSqr = 1.f/dot(fromToUnnormalized, fromToUnnormalized);
	float absDot =  abs(dot(toNormal, fromToUnnormalized * sqrt(invDistSqr)));
    return absDot * invDistSqr;
}


void sampleLightOrEnv(float lightPickRand, float4 lightSampleRand, float envSampleRelativeProbability, out SpectralSamples radianceOut, out float3 posOut, out float3 dirOut, out float pdfOut)
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