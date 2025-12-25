#ifndef BDPT_COMMON_HLSL
#define BDPT_COMMON_HLSL

static float g_spectralMainSampleWavelength;
#define GET_SPECTRAL_SAMPLE_WAVELENGTH g_spectralMainSampleWavelength;

#include "../globalDefinitions.hlsl"
#include "hitShadersCommon.hlsl"
#include "materialSample.hlsl"
#include "rayState.hlsl"
#include "lightSampling.hlsl"

#define LIGHT_PATH_NODE_FLAG_TERMINATE_SECONDARY_WAVELENGTHS (1 << 0)
#define LIGHT_PATH_NODE_FLAG_HIT_FRONT_FACE (1 << 1)
#define LIGHT_PATH_NODE_FLAG_ENV_LIGHT (1 << 2)

struct BidirectionalPathTraceConstants
{
    uint4 lightAndCameraPathConstraints;
	uint2 lightPathsPerDim;
	uint maxVerticesPerLightPath;
	uint maxAllocatedVertices;
    uint maxCameraPathVertices;
};


struct LightPathHeader
{
	uint2 offsetAndCount;
    float sampledWavelength;
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
RWByteAddressBuffer g_lightPathHeaders : register(u2, space3);
RWStructuredBuffer<LightPathNodePacked> g_lightPathVertices : register(u3, space3);
RWByteAddressBuffer g_counters : register(u4, space3);
#else
ByteAddressBuffer g_lightPathHeaders : register(t2, space3);
StructuredBuffer<LightPathNodePacked> g_lightPathVertices : register(t3, space3);
ByteAddressBuffer g_counters : register(t4, space3);
#endif

#define COUNTER_LIGHT_HEADERS_INDEX 0
#define COUNTER_LIGHT_VERTICES_INDEX 1
#define COUNTER_LIGHT_VERTICES_INDEX_SORT 2

#define MAX_LIGHT_PATH_VERTICES_HARD_LIMIT 16

#define INVALID_LIGHT_NODE_INDEX (0xFFFFFFFF)


#define g_maxVerticesPerLightPath g_bdptConstants.maxVerticesPerLightPath
#define g_maxAllocatedVertices g_bdptConstants.maxAllocatedVertices
#define g_lightPathsPerDim g_bdptConstants.lightPathsPerDim
#define g_maxCameraPathVertices g_bdptConstants.maxCameraPathVertices

#define g_lightPathRandomDimensionOffset (g_maxCameraPathVertices * 4 + 4)

#define g_sampleEnvLightProbabilityWeight 0.0f
#define g_effectiveCameraPathNodesRange (g_bdptConstants.lightAndCameraPathConstraints.zw)
#define g_effectiveLightPathNodesRange (g_bdptConstants.lightAndCameraPathConstraints.xy)

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
    uint4 val;
    val.xy = h.offsetAndCount;
    val.zw = uint2(asuint(h.sampledWavelength),0);
    g_lightPathHeaders.Store4((offset * 4) << 2, val);
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
    node.iorPrevious = asfloat(p.data4.y);
    node.iorCurrent = asfloat(p.data4.z);
    return node;
}

uint getNextIndexFromPackedNode(LightPathNodePacked p)
{
    return p.data1.z;
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
    p.data4 = asuint(float4(0, node.iorPrevious, node.iorCurrent, 0));
    
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
    LightPathHeader h;
    uint4 v = g_lightPathHeaders.Load4((index * 4) << 2);
    h.offsetAndCount = v.xy;
    h.sampledWavelength = asfloat(v.z);
    return h;
}

void evaluateSurfaceAndPDFs(in SurfaceDefinition surfaceDef, in PrecalculatedSurfaceData precalculatedSurfaceData, in float samplingProbabilities[LAYER_COUNT], inout BDPTRayState rayState, in float3 woOS, in float3 wiOS, bool triangleHitFrontFace, bool isFromLightSource,
out SpectralSamples weightOut, out float pdfForward, out float pdfBackward)
{

    SpectralSamples weightDummy;

    TransmissionType transmissionType;
    
    uint flags = EVALUATE_FLAGS_NONE;
    if (isFromLightSource)
    {
        flags |= EVALUATE_FLAGS_LIGHT_PATH;

    }
    evaluateSurface(surfaceDef, woOS, wiOS, samplingProbabilities, precalculatedSurfaceData, flags, weightOut, pdfForward, transmissionType);
    
    //This is silly but surface data is not the same when flippiing wo/wi so have to recalculate it here. TODO: recalculate only things that are independant of the direction and calculate wo/wi dependant things only later on
    {
        TransmissionType transmissionTypeDummy;
        bool triangleHitFrontfaceReverseDir = triangleHitFrontFace;
        if (dot(woOS, surfaceDef.geometryNormal) * dot(wiOS, surfaceDef.geometryNormal) < 0)
        {
            triangleHitFrontfaceReverseDir = !triangleHitFrontfaceReverseDir;

        }
        
        getPrecalculatedSurfaceData(surfaceDef, rayState.getCurrentIOR(), rayState.getPreviousIOR(), wiOS, triangleHitFrontfaceReverseDir, precalculatedSurfaceData);
        calculateNormalizedMaterialLayerSamplingProbabilities(surfaceDef, precalculatedSurfaceData.woBase, precalculatedSurfaceData.woCoating, precalculatedSurfaceData.fromIOR, precalculatedSurfaceData.toIOR, precalculatedSurfaceData.a2, samplingProbabilities);
        evaluateSurface(surfaceDef, wiOS, woOS, samplingProbabilities, precalculatedSurfaceData, EVALUATE_FLAGS_PDF_ONLY, weightDummy, pdfBackward, transmissionTypeDummy); //generate pdf for reversed order
    }
    
    

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
    float3 geometryNormal = fetchMeshTriangleNormal(meshEntry.positionBuffer, indices, barycentrics);
    float3 normal = fetchMeshNormal(meshEntry.normalBuffer, indices, barycentrics);
    float3 tangent = meshHasValidTangents(meshEntry.tangentBuffer) ? fetchMeshTangent(meshEntry.tangentBuffer, indices, barycentrics) : float3(1.f, 0.f, 0.f);
    float2 uv = meshHasValidUVs(meshEntry.uvBuffer) ? fetchMeshUV(meshEntry.uvBuffer, indices, barycentrics) : float2(0.5f, 0.5f);


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

void calculateCommonSurfaceParams(in BDPTRayState rayState, SurfaceDefinitionRGB surfaceDefRGB, in float3 woOS, bool triangleHitFrontFace,
out SurfaceDefinition surfaceDef, out PrecalculatedSurfaceData precalculatedSurfaceData, out float samplingProbabilities[LAYER_COUNT])
{

    surfaceDef = convertSurfaceDefinitionFromRGB(surfaceDefRGB);

    getPrecalculatedSurfaceData(surfaceDef, rayState.getCurrentIOR(), rayState.getPreviousIOR(), woOS, triangleHitFrontFace, precalculatedSurfaceData);
    calculateNormalizedMaterialLayerSamplingProbabilities(surfaceDef, precalculatedSurfaceData, samplingProbabilities);
}


void calculateCommonSurfaceParams(in BDPTRayState rayState, in uint instanceIndex, in uint primitiveIndex, in float2 barycentrics2, in float3 woOS, bool triangleHitFrontFace,
out SurfaceDefinition surfaceDef, out PrecalculatedSurfaceData precalculatedSurfaceData, out float samplingProbabilities[LAYER_COUNT])
{
    SurfaceDefinitionRGB surfaceDefRGB;
    fillSurfaceDefRGB(instanceIndex, primitiveIndex, barycentrics2, surfaceDefRGB);
	
    calculateCommonSurfaceParams(rayState, surfaceDefRGB, woOS, triangleHitFrontFace, surfaceDef, precalculatedSurfaceData, samplingProbabilities);
}


void pdfForSamplingLightNode(LightPathNode lightNode, float3 towardsDir, float envSampleRelativeProbability, out float lightPickPDF, out float posPDF, out float dirPDF)
{
    pdfForSamplingLight(lightNode.instanceIndex, lightNode.primitiveIndex, lightNode.barycentrics, lightNode.normalWS, towardsDir, envSampleRelativeProbability, lightPickPDF, posPDF, dirPDF);
}

void sampleLightOrEnv(float lightPickRand, float4 lightSampleRand0, float2 lightSampleRand1, float envSampleRelativeProbability, out SpectralSamples radianceOut, out float3 posOut, out float3 dirOut, out float3 normalOut, out float pdfPosOut, out float pdfDirOut, out float pdflightSelection, out bool sampledEnvironment, out uint instanceOut, out uint primitiveOut)
{
    if (g_lightCount == 0)
    {
        sampleEnvironmentLighting(lightSampleRand0, radianceOut, posOut, dirOut, pdfPosOut, pdfDirOut);
        pdflightSelection = 1.f;
        sampledEnvironment = true;
        normalOut = 0;
        instanceOut = -1;
        primitiveOut = -1;
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
        instanceOut = -1;
        primitiveOut = -1;

    }
    else
    {
        uint lightIndex = min((uint) floor(lightPickRand * g_lightCount), g_lightCount - 1);
        sampleLight(lightIndex, lightSampleRand0.xyz, lightSampleRand1, radianceOut, posOut, dirOut, normalOut, pdfPosOut, pdfDirOut, instanceOut, primitiveOut);
        pdflightSelection = 1 / lightProb;
        sampledEnvironment = false;

    }
}


#endif