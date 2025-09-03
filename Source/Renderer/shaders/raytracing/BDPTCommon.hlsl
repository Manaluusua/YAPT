#ifndef BDPT_COMMON_HLSL
#define BDPT_COMMON_HLSL

#include "../globalDefinitions.hlsl"
#include "materialSample.hlsl"
#include "rayState.hlsl"
#include "../common/miscBrdf.hlsl"

#define LIGHT_PATH_NODE_FLAG_TERMINATE_SECONDARY_WAVELENGTHS (1 << 0)
#define LIGHT_PATH_NODE_FLAG_HIT_FRONT_FACE (1 << 1)
#define LIGHT_PATH_NODE_FLAG_ENV_LIGHT (1 << 2)

struct BidirectionalPathTraceConstants
{
	float4 worldBoundsMin;
	float4 worldBoundsMax;
	uint2 lightPathsPerDim;
	uint envTextureIndex;
	uint envType;
	uint maxVerticesPerLightPath;
	uint maxAllocatedVertices;
    uint maxCameraPathVertices;
};


struct LightPathHeader
{
	uint2 offsetAndCount;
};

struct LightPathNodePacked
{
    uint4 data0;
    uint4 data1;
    uint4 data2;
    uint4 data3;
    uint4 data4;
};

struct LightPathNode
{
    float4 radiance;
    float3 normalWS;
    uint nextIndex;
    float3 positionWS;
    uint instanceIndex;
    float2 barycentrics;
    uint primitiveIndex;
    float pdfForwardMIS;
    float pdfBackwardMIS;
    float iorPrevious;
    float iorCurrent;
    float riSum;
    uint flags;

};

struct ExtractedLightPathNodeData
{
    LightPathNode node;
    SurfaceDefinitionRGB surfaceDefRGB;
    float3x4 worldToObjSpace;
};

struct BDPTRayState //: RayStateInterface
{

    SpectralSamples getAbsorption()
    {
        if (numberVolumesEntered != 0)
        {
            return absorption[numberVolumesEntered - 1];
        } else
        {
            SpectralSamples abs;
            abs.set(0);
            return abs;

        }
    }
    
    float getCurrentIOR()
    {
        float currentIOR = IOR_DEFAULT; //air if not entered volume
        if (numberVolumesEntered != 0)
        {
            currentIOR = ior[numberVolumesEntered - 1];
        }
        return currentIOR;
    }
    float getPreviousIOR()
    {
        float beforeCurrentIOR = IOR_DEFAULT;
        if (numberVolumesEntered > 1)
        {
            beforeCurrentIOR = ior[numberVolumesEntered - 2];
        }
        return beforeCurrentIOR;
    }

    void enteredVolume(float IOR, SpectralSamples absorptionParam)
    {
        numberVolumesEntered = min(numberVolumesEntered + 1, RAY_MAX_VOLUMES_ENTERED);
        ior[numberVolumesEntered - 1] = IOR;
        absorption[numberVolumesEntered - 1] = absorptionParam;
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

    SpectralSamples absorption[RAY_MAX_VOLUMES_ENTERED];
    float ior[RAY_MAX_VOLUMES_ENTERED];
    uint numberVolumesEntered;
    uint flags;
};



ConstantBuffer<BidirectionalPathTraceConstants> g_bdptConstants : register(b0, space3);

RaytracingAccelerationStructure g_accelerationStructure : register(t1, space3);

#ifdef WRITABLE_LIGHT_DATA
RWStructuredBuffer<LightPathHeader> g_lightPathHeaders : register(u2, space3);
RWStructuredBuffer<LightPathNodePacked> g_lightPathVertices : register(u3, space3);
RWByteAddressBuffer g_counters : register(u4, space3);
#else
StructuredBuffer<LightPathHeader> g_lightPathHeaders : register(t2, space3);
StructuredBuffer<LightPathNodePacked> g_lightPathVertices : register(t3, space3);
ByteAddressBuffer g_counters : register(t4, space3);
#endif

#define COUNTER_LIGHT_HEADERS_INDEX 0
#define COUNTER_LIGHT_VERTICES_INDEX 1
#define COUNTER_LIGHT_VERTICES_INDEX_SORT 2

#define MAX_LIGHT_PATH_VERTICES_HARD_LIMIT 16

#define INVALID_LIGHT_NODE_INDEX (0xFFFFFFFF)

#define g_worldBoundsMin g_bdptConstants.worldBoundsMin.xyz
#define g_worldBoundsMax g_bdptConstants.worldBoundsMax.xyz
#define g_maxVerticesPerLightPath g_bdptConstants.maxVerticesPerLightPath
#define g_maxAllocatedVertices g_bdptConstants.maxAllocatedVertices
#define g_lightPathsPerDim g_bdptConstants.lightPathsPerDim
#define g_maxCameraPathVertices g_bdptConstants.maxCameraPathVertices

#define g_envType g_bdptConstants.envType
#define g_envTexIndex g_bdptConstants.envTextureIndex

#define g_sampleEnvLightProbabilityWeight 0.5f

#ifdef WRITABLE_LIGHT_DATA

void reserveLightPathNodeSpace(uint count, out uint offsetOut, out uint countOut)
{
    g_counters.InterlockedAdd(COUNTER_LIGHT_VERTICES_INDEX << 2, count, offsetOut);
    uint actualCount = g_maxAllocatedVertices - min(offsetOut, g_maxAllocatedVertices);
    offsetOut = offsetOut;
    countOut = actualCount;
}

void reserveLightPathNodeSpaceForSorting(uint count, out uint offsetOut)
{
    g_counters.InterlockedAdd(COUNTER_LIGHT_VERTICES_INDEX_SORT << 2, count, offsetOut);
    offsetOut = offsetOut;
}

void storeLightPathVertex(uint offset, LightPathNodePacked node)
{
	g_lightPathVertices[offset] = node;
}

uint reserveLightPathHeader()
{
	uint offset;
    g_counters.InterlockedAdd(COUNTER_LIGHT_HEADERS_INDEX << 2, 1, offset);
    return offset;
}

void storeLightPathHeader(uint offset, LightPathHeader h)
{
	g_lightPathHeaders[offset] = h;
}



#endif


LightPathNode decompressLightPathNode(LightPathNodePacked p)
{
    LightPathNode node;
    node.radiance = asfloat(p.data0);
    node.normalWS = float3(f16tof32(p.data1.x & 0xFFFF), f16tof32(p.data1.x >> 16), f16tof32(p.data1.y & 0xFFFF));
    node.flags = p.data1.y >> 16;
    node.nextIndex = p.data1.z;
    node.instanceIndex = p.data1.w;
    node.positionWS = asfloat(p.data2.xyz);
    node.primitiveIndex = p.data2.w;
    node.barycentrics = asfloat(p.data3.xy);
    node.pdfForwardMIS = asfloat(p.data3.z);
    node.pdfBackwardMIS = asfloat(p.data3.w);
    node.riSum = asfloat(p.data4.x);
    node.iorPrevious = asfloat(p.data4.y);
    node.iorCurrent = asfloat(p.data4.z);
    return node;
}

LightPathNodePacked compressLightPathNode(LightPathNode node)
{
    LightPathNodePacked p;
    p.data0 = asuint(node.radiance);
    p.data1 = uint4(f32tof16(node.normalWS.x) | f32tof16(node.normalWS.y) << 16,
                    f32tof16(node.normalWS.z) | node.flags << 16,
                    node.nextIndex, node.instanceIndex);
    p.data2 = uint4(asuint(node.positionWS.x), asuint(node.positionWS.y), asuint(node.positionWS.z), node.primitiveIndex);
    p.data3 = asuint(float4(node.barycentrics.x, node.barycentrics.y, node.pdfForwardMIS, node.pdfBackwardMIS));
    p.data4 = uint4(node.riSum, node.iorPrevious, node.iorCurrent, 0);
    
    return p;
}

LightPathNodePacked getLightPathVertex(uint offset)
{
    return g_lightPathVertices[offset];
}

uint getLightPathsCount()
{
    return g_counters.Load(COUNTER_LIGHT_HEADERS_INDEX << 2);

}

LightPathHeader getLightPathHeader(uint index)
{
    return g_lightPathHeaders[index];
}


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

    TransmissionType transmissionType;
    TransmissionType transmissionTypeDummy;
	
    evaluateSurface(surfaceDef, woOS, wiOS, samplingProbabilities, precalculatedSurfaceData, false, weightOut, pdfForward, transmissionType);
    evaluateSurface(surfaceDef, wiOS, woOS, samplingProbabilities, precalculatedSurfaceData, true, weightDummy, pdfBackward, transmissionTypeDummy); //generate pdf for reversed order

    if (transmissionType != TRANSMISSION_TYPE_NONE)
    {
        if ((transmissionType & TRANSMISSION_TYPE_DISPERSED) != 0)
        {
            rayState.setStateFlags(rayState.getStateFlags() | RAYSTATE_FLAGS_SECONDARY_LAMBDAS_TERMINATED);
        }
			
        if ((transmissionType & TRANSMISSION_TYPE_EXITED) != 0)
        {
            rayState.exitedVolume();

        }
        else if ((transmissionType & TRANSMISSION_TYPE_ENTERED) != 0)
        {
            rayState.enteredVolume(surfaceDef.dielectricIOR, surfaceDef.absorption);
        }
    }
}

void fillSurfaceDefRGB(in uint instanceIndex, in uint primitiveIndex, in float2 barycentrics2, out SurfaceDefinitionRGB surfaceDefOut)
{
    uint2 matMeshIndices = getMaterialAndMeshIndicesForInstance(instanceIndex);
    MeshEntryGPU meshEntry = getMeshEntry(matMeshIndices.y);

	//Initial surface setup
    float3 barycentrics = float3(1 - barycentrics2.x - barycentrics2.y, barycentrics2.x, barycentrics2.y);
    uint3 indices = fetchIndices(meshEntry.indexBuffer, primitiveIndex);
    float3 geometryNormal = fetchMeshNormal(meshEntry.normalBuffer, indices, barycentrics);
    float3 tangent = meshHasValidTangents(meshEntry.tangentBuffer) ? fetchMeshTangent(meshEntry.tangentBuffer, indices, barycentrics) : float3(1.f, 0.f, 0.f);
    float2 uv = meshHasValidUVs(meshEntry.uvBuffer) ? fetchMeshUV(meshEntry.uvBuffer, indices, barycentrics) : float2(0.5f, 0.5f);

    float3 normal = geometryNormal;

	//fetch surface material parameters
    MaterialEntryGPU matEntry = getMaterialEntry(matMeshIndices.x);
    SurfaceDefinitionRGB surfaceDefRGB;
    fetchSurfaceMaterialParameters(matEntry, surfaceDefRGB);
    modifySurfaceMaterialParametersWithTextures(matEntry, uv, normal, tangent, surfaceDefRGB);
    setupSurfaceOrientation(geometryNormal, normal, normal, tangent, surfaceDefRGB);
    
    surfaceDefOut = surfaceDefRGB;
}

ExtractedLightPathNodeData getExtractedLightPathNodeData(LightPathNode node)
{
    ExtractedLightPathNodeData data;
    data.node = node;
    fillSurfaceDefRGB(node.instanceIndex, node.primitiveIndex, node.barycentrics, data.surfaceDefRGB);
    data.worldToObjSpace = getTransformDataForInstance(node.instanceIndex).getWorldToObj();
    return data;
}

void calculateCommonSurfaceParams(in BDPTRayState rayState, in uint instanceIndex, in uint primitiveIndex, in float2 barycentrics2, in float3 woOS, bool triangleHitFrontFace, in float2 materialLayerRands,
out SurfaceDefinition surfaceDef, out PrecalculatedSurfaceData precalculatedSurfaceData, out float samplingProbabilities[LAYER_COUNT])
{
    SurfaceDefinitionRGB surfaceDefRGB;
    fillSurfaceDefRGB(instanceIndex, primitiveIndex, barycentrics2, surfaceDefRGB);
    
    surfaceDefRGB.coatingLayerNormal = nudgeNormal(-woOS, surfaceDefRGB.coatingLayerNormal, surfaceDefRGB);
    surfaceDefRGB.baseLayerNormal = nudgeNormal(-woOS, surfaceDefRGB.baseLayerNormal, surfaceDefRGB);
    surfaceDefRGB.geometryNormal = nudgeNormal(-woOS, surfaceDefRGB.geometryNormal, surfaceDefRGB);
	    
    surfaceDef = convertSurfaceDefinitionFromRGB(surfaceDefRGB);

    getPrecalculatedSurfaceData(surfaceDef, rayState.getCurrentIOR(), rayState.getPreviousIOR(), woOS, triangleHitFrontFace, precalculatedSurfaceData);
    calculateNormalizedMaterialLayerSamplingProbabilities(surfaceDef, precalculatedSurfaceData, materialLayerRands, samplingProbabilities);
}


void sampleEnvironmentLighting(float4 randValues, out SpectralSamples radianceOut, out float3 posOut, out float3 dirOut, out float pdfPosOut, out float pdfDirOut)
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
    pdfPosOut = pdfPos;
    pdfDirOut = pdfDir;

}

void sampleLight(uint lightIndex, float4 randValues0, float2 randValues1, out SpectralSamples radianceOut, out float3 posOut, out float3 dirOut, out float3 normalOut, out float pdfPosOut, out float pdfDirOut)
{
	LightEntryGPU lightEntry = g_lights[lightIndex];
	MeshEntryGPU meshEntry = getMeshEntry(lightEntry.meshIndex);

	uint primCount = (meshEntry.indexCount / 3);
    uint primitiveIndex = min(primCount * randValues0.z, primCount - 1);

    float3 barycentrics = float3(1 - randValues0.x - randValues0.y, randValues0.x, randValues0.y);
	uint3 indices = fetchIndices(meshEntry.indexBuffer, primitiveIndex);

	float3 normal = fetchMeshNormal(meshEntry.normalBuffer, indices, barycentrics);
	float3 tangent = meshHasValidTangents(meshEntry.tangentBuffer) ? fetchMeshTangent(meshEntry.tangentBuffer, indices, barycentrics) : float3(1.f, 0.f, 0.f);
	float2 uv = meshHasValidUVs(meshEntry.uvBuffer) ? fetchMeshUV(meshEntry.uvBuffer, indices, barycentrics) : float2(0.5f, 0.5f);

	MaterialEntryGPU matEntry = getMaterialEntry(lightEntry.matIndex);
	SurfaceDefinitionRGB surfaceDefRGB;
	fetchSurfaceMaterialParameters(matEntry, surfaceDefRGB);
	modifySurfaceMaterialParametersWithTextures(matEntry, uv, normal, tangent, surfaceDefRGB);

	normal = mul(lightEntry.transformInvTransp, float4(normal, 0.f)).xyz;
    tangent = mul(lightEntry.transformInvTransp, float4(tangent, 0.f)).xyz;
    
    float3x3 tanToWS = constructBasisTransform(normal, tangent);
    float3 lightDir = sampleHemisphere(randValues1);
    lightDir = mul(tanToWS, lightDir);

	float3 p1, p2, p3;
	fetchMeshPositions(meshEntry.positionBuffer, indices, p1, p2, p3);
	p1 = mul(lightEntry.transform, float4(p1, 1)).xyz;
	p2 = mul(lightEntry.transform, float4(p2, 1)).xyz;
	p3 = mul(lightEntry.transform, float4(p3, 1)).xyz;
	float area = length(cross(p2 - p1, p3 - p1)) * 0.5f;
	float3 pos = barycentrics.x * p1 + barycentrics.y * p2 + barycentrics.z * p3;

	radianceOut.setFromRGBUnbounded(surfaceDefRGB.emissive);
    
    pdfPosOut = 1.f / (primCount * area);
    pdfDirOut = pdfHemisphere();
    
	posOut = pos;
    dirOut = lightDir;
    normalOut = normal;

}

float areaDensityMultiplier(float3 fromToUnnormalized, float3 toNormal)  
{
	float invDistSqr = 1.f/dot(fromToUnnormalized, fromToUnnormalized);
	float absDot =  abs(dot(toNormal, fromToUnnormalized * sqrt(invDistSqr)));
    return absDot * invDistSqr;
}


void sampleLightOrEnv(float lightPickRand, float4 lightSampleRand0, float2 lightSampleRand1, float envSampleRelativeProbability, out SpectralSamples radianceOut, out float3 posOut, out float3 dirOut, out float3 normalOut, out float pdfPosOut, out float pdfDirOut, out float pdflightSelection, out bool sampledEnvironment)
{
	if (g_lightCount == 0)
	{
        sampleEnvironmentLighting(lightSampleRand0, radianceOut, posOut, dirOut, pdfPosOut, pdfDirOut);
        pdflightSelection = 1.f;
        sampledEnvironment = true;
        normalOut = 0;
        return;
    }

	float lightProb = g_lightCount + envSampleRelativeProbability;
	float s = lightPickRand * lightProb;
	if (s > g_lightCount)
	{
        sampleEnvironmentLighting(lightSampleRand0, radianceOut, posOut, dirOut, pdfPosOut, pdfDirOut);
        pdflightSelection = envSampleRelativeProbability / lightProb;
        sampledEnvironment = true;
        normalOut = 0;

    }
	else
	{
		uint lightIndex = min((uint)floor(lightPickRand * g_lightCount), g_lightCount - 1);
        sampleLight(lightIndex, lightSampleRand0, lightSampleRand1, radianceOut, posOut, dirOut, normalOut, pdfPosOut, pdfDirOut);
        pdflightSelection = 1 / lightProb;
        sampledEnvironment = false;

    }
}

void pdfForSamplingLightNode(LightPathNode lightNode, float envSampleRelativeProbability,  out float lightPickPDF, out float posPDF, out float dirPDF)
{

    float lightProb = g_lightCount + envSampleRelativeProbability;
    lightPickPDF = 1.f / lightProb;
    //TODO: refactor light sampling to separate PDF calculation

}

void pdfForSamplingEnv(float envSampleRelativeProbability, out float lightPickPDF, out float posPDF, out float dirPDF)
{

    float lightProb = g_lightCount + envSampleRelativeProbability;
    lightPickPDF = envSampleRelativeProbability / lightProb;
    //TODO: refactor light sampling to separate PDF calculation
    
}


#endif