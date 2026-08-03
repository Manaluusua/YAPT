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
#define LIGHT_PATH_NODE_FLAG_IS_DELTA_DISTRIBUTION (1 << 3)

struct BidirectionalPathTraceConstants
{
    uint4 lightAndCameraPathConstraints;
	uint2 lightPathsPerDim;
    uint2 cameraRayWorkGroupCount;
    uint cameraWorkGroupSwizzleOffset;
	uint maxVerticesPerLightPath;
	uint maxAllocatedVertices;
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
        numberVolumesEntered = numberVolumesEntered == 0 ? 0 : numberVolumesEntered - 1;
    }

    uint getStateFlags()
    {
        return flags;
    }
    void setStateFlags(uint flagsIn)
    {
        flags = flagsIn;
    }

    void addFlags(uint flagsIn)
    {
        setStateFlags(getStateFlags() | flagsIn);
    }
    
    void removeFlags(uint flagsIn)
    {
        setStateFlags(getStateFlags() & ~flagsIn);
    }
    
    bool hasFlags(uint flagsIn)
    {
        return (getStateFlags() & flagsIn) == flagsIn;

    }

    SpectralSamples absorption[RAY_MAX_VOLUMES_ENTERED];
    float4 ior;
    uint numberVolumesEntered;
    uint flags;
};



ConstantBuffer<BidirectionalPathTraceConstants> g_bdptConstants : register(b0, space3);

RaytracingAccelerationStructure g_accelerationStructure : register(t1, space3);

#ifdef WRITABLE_LIGHT_DATA
RWByteAddressBuffer g_lightPathHeaders : register(u2, space3);
RWStructuredBuffer<LightPathNodePacked> g_lightPathVertices : register(u3, space3);
#else
ByteAddressBuffer g_lightPathHeaders : register(t2, space3);
StructuredBuffer<LightPathNodePacked> g_lightPathVertices : register(t3, space3);
#endif

#ifdef WRITABLE_COUNTERS
RWByteAddressBuffer g_counters : register(u4, space3);
#else
ByteAddressBuffer g_counters : register(t4, space3);
#endif

#define COUNTER_LIGHT_HEADERS_INDEX 0
#define COUNTER_LIGHT_VERTICES_INDEX 1
#define COUNTER_LIGHT_VERTICES_INDEX_SORT 2
#define COUNTER_LIGHT_VERTICES_PURE_NODE_INDEX 3

#define MAX_LIGHT_PATH_VERTICES_HARD_LIMIT 16

#define INVALID_LIGHT_NODE_INDEX (0xFFFFFFFF)


#define g_maxVerticesPerLightPath g_bdptConstants.maxVerticesPerLightPath
#define g_maxAllocatedVertices g_bdptConstants.maxAllocatedVertices
#define g_lightPathsPerDim g_bdptConstants.lightPathsPerDim

#define g_cameraPathRandomDimensionOffset (g_maxVerticesPerLightPath * 4 + 4)
#define g_cameraRayWorkGroupCount (g_bdptConstants.cameraRayWorkGroupCount)
#define g_cameraWorkGroupOffset (g_bdptConstants.cameraWorkGroupSwizzleOffset)

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

void evaluateSurfaceAndPDFs(in SurfaceDefinition surfaceDef, in PrecalculatedSurfaceData precalculatedSurfaceData, in float samplingProbabilities[LAYER_COUNT], inout BDPTRayState rayState, in float3 woOS, in float3 wiOS, bool triangleHitFrontFace, bool isFromLightSource, bool treatAsDelta,
out SpectralSamples weightOut, out float pdfForward, out float pdfBackward)
{

    SpectralSamples weightDummy;

    TransmissionType transmissionType;
    
    uint flags = EVALUATE_FLAGS_NONE;
    if (isFromLightSource)
    {
        flags |= EVALUATE_FLAGS_LIGHT_PATH;
    }
    if (treatAsDelta)
    {
        flags |= EVALUATE_FLAGS_TREAT_AS_DELTA;
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

float calculatePDFRatio(float nom, float denom)
{
    nom = nom < 0 ? 1.f : nom;
    denom = denom < 0 ? 1.f : denom;
    return safeDiv(nom, denom);
}

float accumulateRISum(float riSum, float nom, float denom)
{
    float ri = calculatePDFRatio(nom, denom);
    bool accumulateRI = nom > 0 && denom > 0;
    riSum = ri * riSum;
    if(accumulateRI)
    {
        riSum += ri;
    }
    return riSum;
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
    fetchSurfaceMaterialParameters(matEntry, surfaceDefRGB, geometryNormal, normal, normal, tangent);
    setupSurfaceOrientation(surfaceDefRGB);
    modifySurfaceMaterialParametersWithTextures(matEntry, uv, normal, tangent, surfaceDefRGB);
    
    
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

    handleTwoSidedMaterialOrientation(surfaceDefRGB, woOS);
    
    surfaceDef = convertSurfaceDefinitionFromRGB(surfaceDefRGB);
    
    if ((rayState.getStateFlags() & RAYSTATE_FLAGS_HAS_DIFFUSE_BOUNCE) != 0)
    {
        regularizeMaterial(surfaceDefRGB);
    }

    getPrecalculatedSurfaceData(surfaceDef, rayState.getCurrentIOR(), rayState.getPreviousIOR(), woOS, triangleHitFrontFace, precalculatedSurfaceData);
    calculateNormalizedMaterialLayerSamplingProbabilities(surfaceDef, precalculatedSurfaceData, samplingProbabilities);
}


void calculateCommonSurfaceParams(in BDPTRayState rayState, in uint instanceIndex, in uint primitiveIndex, in float2 barycentrics2, in float3 woOS, bool triangleHitFrontFace,
out SurfaceDefinition surfaceDef, out PrecalculatedSurfaceData precalculatedSurfaceData, out float samplingProbabilities[LAYER_COUNT])
{
    SurfaceDefinitionRGB surfaceDefRGB;
    fillSurfaceDefRGB(instanceIndex, primitiveIndex, barycentrics2, surfaceDefRGB);
    if ((rayState.getStateFlags() & RAYSTATE_FLAGS_HAS_DIFFUSE_BOUNCE) != 0)
    {
        regularizeMaterial(surfaceDefRGB);
    }
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
        LightSampleOutput output;
        sampleLight(lightIndex, lightSampleRand0.xyz, lightSampleRand1, output);
        
        radianceOut = output.radiance;
        posOut = output.positionWS;
        dirOut = output.directionWS;
        normalOut = output.normalWS;
        pdfPosOut = output.pdfPos;
        pdfDirOut = output.pdfDir;
        instanceOut = output.instanceIndex;
        primitiveOut = output.primitiveIndex;
        pdflightSelection = 1 / lightProb;
        sampledEnvironment = false;

    }
}


bool calculateConnectingLightNodeWeightsAndPDFs(ExtractedLightPathNodeData lightNodeData, uint lightNodeIndex, float3 prevVertexPos, float3 nextVertexPos, out float pdfForwardOut, out float pdfBackwardOut, out SpectralSamples weightOut, out float pdfForwardMISOut)
{

    SurfaceDefinitionRGB surfaceDefRGB = lightNodeData.surfaceDefRGB;
    float3x4 toLocalFrame = lightNodeData.worldToObjSpace;
    LightPathNode lightNode = lightNodeData.node;
    uint lightNodeFlags = asuint(lightNode.flags);
    bool triangleHitFrontFace = (lightNodeFlags & LIGHT_PATH_NODE_FLAG_HIT_FRONT_FACE) != 0;
    if (lightNodeIndex == 0)
    {
        bool twoSided = isSurfaceTwoSided(surfaceDefRGB.flags);
        float3 fromLightWS = nextVertexPos - lightNode.positionWS;
        if (dot(fromLightWS, lightNode.normalWS) <= 0 && !twoSided)
        {
            pdfForwardOut = 0;
            pdfBackwardOut = 0;
            pdfForwardMISOut = 0;
            weightOut.fromFloat4(float4(0, 0, 0, 0));
            return false;
        }

		//for first node, this is just pdf for selecting light, ie 1.f/probLightPick*probPos
        float pdf = lightNode.pdfForwardMIS;
        pdfForwardOut = pdf * areaDensityToSolidAngleMultiplier(-fromLightWS, lightNode.normalWS);

        weightOut.set(safeDiv(1.f, pdfForwardOut));
        pdfBackwardOut = 0;

        float cosLightEmit = dot(lightNode.normalWS, normalize(fromLightWS));
        pdfForwardMISOut = twoSided ? pdfCosineWeightedSphere(cosLightEmit) : (cosLightEmit >= 0 ? pdfCosineWeightedHemisphere(cosLightEmit) : 0);


    }
    else
    {

        float3 lightPosOS = mul(toLocalFrame, float4(lightNode.positionWS.xyz, 1)).xyz;
        float3 prevLightPosOS = mul(toLocalFrame, float4(prevVertexPos, 1)).xyz;
        float3 camNodePosOS = mul(toLocalFrame, float4(nextVertexPos, 1)).xyz;
        float3 wiOS = normalize(camNodePosOS - lightPosOS);
        float3 woOSLightConnection = normalize(prevLightPosOS - lightPosOS);


        SurfaceDefinition surfaceDef = convertSurfaceDefinitionFromRGB(surfaceDefRGB);
        PrecalculatedSurfaceData precalculatedSurfaceData;
        float samplingProbabilities[LAYER_COUNT];

		//for now just initialize with node data. maybe needs a better way (update during light node traversal?)
        BDPTRayState rayState;
        rayState.ior[0] = lightNode.iorPrevious;
        rayState.ior[1] = lightNode.iorCurrent;
        rayState.numberVolumesEntered = 2;
        rayState.setStateFlags(0);

        getPrecalculatedSurfaceData(surfaceDef, rayState.getCurrentIOR(), rayState.getPreviousIOR(), woOSLightConnection, triangleHitFrontFace, precalculatedSurfaceData);
        calculateNormalizedMaterialLayerSamplingProbabilities(surfaceDef, precalculatedSurfaceData, samplingProbabilities);
        evaluateSurfaceAndPDFs(surfaceDef, precalculatedSurfaceData, samplingProbabilities, rayState, woOSLightConnection, wiOS, triangleHitFrontFace, true, false, weightOut, pdfForwardOut, pdfBackwardOut);

        pdfForwardMISOut = pdfForwardOut;

        if (pdfBackwardOut == 0 || weightOut.allSamplesEqual(0))
        {
            return false;
        }
    }

    return true;
}

struct CameraVertexContext
{
    SurfaceDefinition surfaceDef;
    PrecalculatedSurfaceData precalculatedSurfaceData;
    SpectralSamples throughput;
    float samplingProbabilities[LAYER_COUNT];
    float3 posWS;
    float pdfRatioSum;
    float3 normalWS;
    float3 woOS;
    float3 wiOS;
	
    float3 prevNodePosWS;
    float previousPDFForwardMIS;
    float3 prevNodeNormalWS;
    float currentNodeForwardPDFMIS;
    bool triangleHitFrontFace;
};

struct LightVertexContext
{
    float3 prevNodePosWS;
    float previousPDFForwardMIS;
    float3 prevNodeNormalWS;
    uint originatingNodeFlags;
    float riSum;
};


float calculateMISWeight(uint cameraVertexIndex, in CameraVertexContext camVertex,
uint lightVertexIndex, in LightVertexContext lightVertex, in LightPathNode lightNode,
float pdfBackwardLastCameraNode, float pdfForwardLastCameraNode, float pdfBackwardLastLightNode, float pdfForwardLastLightNode)
{

    float riSumCam = camVertex.pdfRatioSum;
	//TODO: the actual first node would be the origina of the ray from camera but its not considered (and can never be hit by path from light). Revisit if its correct to just ignore it here
    if (cameraVertexIndex > 1)
    {
        float pdfBackwardPrevNodeCam = pdfBackwardLastCameraNode * solidAngleToAreaDensityMultiplier(camVertex.prevNodePosWS - camVertex.posWS, camVertex.prevNodeNormalWS);
        riSumCam = accumulateRISum(riSumCam, pdfBackwardPrevNodeCam, camVertex.previousPDFForwardMIS);
    }
    if (cameraVertexIndex > 0)
    {
        riSumCam = accumulateRISum(riSumCam, pdfForwardLastLightNode * solidAngleToAreaDensityMultiplier(camVertex.posWS - lightNode.positionWS.xyz, camVertex.normalWS), camVertex.currentNodeForwardPDFMIS);
    }

	
    float riSumLight = lightVertex.riSum;
    if (lightVertexIndex == 0)
    {
        bool isEnvLight = (lightNode.flags & LIGHT_PATH_NODE_FLAG_ENV_LIGHT) != 0;
        float lightPickPDF;
        float lightPosPDF;
        float lightDirPDF;
		
        float3 camToLight = lightNode.positionWS.xyz - camVertex.posWS;
        float camToLightLength = length(camToLight);
        float3 lightToCam = -camToLight / camToLightLength;
		
        float misWeightToLight;
        if (isEnvLight)
        {
            pdfForSamplingEnv(g_sampleEnvLightProbabilityWeight, lightToCam, lightPickPDF, lightPosPDF, lightDirPDF);
            misWeightToLight = pdfForwardLastCameraNode;
        }
        else
        {
            pdfForSamplingLightNode(lightNode, lightToCam, g_sampleEnvLightProbabilityWeight, lightPickPDF, lightPosPDF, lightDirPDF);
            misWeightToLight = pdfForwardLastCameraNode * solidAngleToAreaDensityMultiplier(camToLight, lightNode.normalWS);
        }
		
        riSumLight = calculatePDFRatio(misWeightToLight, (lightPickPDF * lightPosPDF));
		
    }
    else
    {
        float pdfBackwardPrevNodeLight = pdfBackwardLastLightNode;

        bool prevNodeIsEnvLight = (lightVertexIndex == 1) && ((lightVertex.originatingNodeFlags & LIGHT_PATH_NODE_FLAG_ENV_LIGHT) != 0);
        if (!prevNodeIsEnvLight)
        {
            pdfBackwardPrevNodeLight *= solidAngleToAreaDensityMultiplier(lightVertex.prevNodePosWS - lightNode.positionWS, lightVertex.prevNodeNormalWS);
        }
		
        riSumLight = accumulateRISum(riSumLight, pdfBackwardPrevNodeLight, lightVertex.previousPDFForwardMIS);
        riSumLight = accumulateRISum(riSumLight, pdfForwardLastCameraNode * solidAngleToAreaDensityMultiplier(lightNode.positionWS.xyz - camVertex.posWS, lightNode.normalWS), lightNode.pdfForwardMIS);
    }
	
    float misWeight = 1.f / (1 + riSumCam + riSumLight);

    return misWeight;
}

float calculateMISWeightOnCameraPathHittingLight(uint cameraVertexIndex, uint instanceIndex, uint primitiveIndex, float2 bary, CameraVertexContext camVertexContext)
{
	//if we hit directly to light from camera (or one bounce), this path can only be done this way and MIS is 1.
    if (cameraVertexIndex == 0)
    {
        return 1.f;
    }

    float lightPickPDF;
    float lightPosPDF;
    float lightDirPDF;
	
    float3 towardsPreviousCameraNode = camVertexContext.prevNodePosWS - camVertexContext.posWS;
    float3 towardsPreviousCameraNodeDir = normalize(towardsPreviousCameraNode);
    pdfForSamplingLight(instanceIndex, primitiveIndex, bary, camVertexContext.normalWS, towardsPreviousCameraNodeDir, g_sampleEnvLightProbabilityWeight, lightPickPDF, lightPosPDF, lightDirPDF);
	
    float riSumCam = camVertexContext.pdfRatioSum;
   
    if (cameraVertexIndex > 1)
    {
        riSumCam = accumulateRISum(riSumCam, lightDirPDF * solidAngleToAreaDensityMultiplier(towardsPreviousCameraNode, camVertexContext.prevNodeNormalWS), camVertexContext.previousPDFForwardMIS);
    }

    riSumCam = accumulateRISum(riSumCam, lightPickPDF * lightPosPDF, camVertexContext.currentNodeForwardPDFMIS);

    float misWeight = 1.f / (1 + riSumCam);
    return misWeight;
}

float calculateMISWeightOnCameraPathMissing(uint cameraVertexIndex, CameraVertexContext context, float3 rayOrigin, float3 rayDir)
{
	//if we hit directly to light from camera, this path can only be done this way and MIS is 1
    if (cameraVertexIndex == 0)
    {
        return 1.f;
    }

    float lightPickPDF;
    float lightPosPDF;
    float lightDirPDF;

    pdfForSamplingEnv(g_sampleEnvLightProbabilityWeight, -rayDir, lightPickPDF, lightPosPDF, lightDirPDF);

    float3 towardsPreviousCameraNode = context.prevNodePosWS - context.posWS;
    float3 towardsPreviousCameraNodeDir = normalize(towardsPreviousCameraNode);

    float riSumCam = context.pdfRatioSum;
   
    if (cameraVertexIndex > 1)
    {
        riSumCam = accumulateRISum(riSumCam, lightDirPDF * solidAngleToAreaDensityMultiplier(towardsPreviousCameraNode, context.prevNodeNormalWS), context.previousPDFForwardMIS);
    }

    riSumCam = accumulateRISum(riSumCam, lightPickPDF * lightPosPDF, context.currentNodeForwardPDFMIS);

    float misWeight = 1.f / (1 + riSumCam);
    return misWeight;

}


SpectralSamples calculateLightPathRadiance(uint cameraVertexIndex, in CameraVertexContext camVertex,
ExtractedLightPathNodeData lightNodeData, uint lightVertexIndex, in LightVertexContext lightVertex, in BDPTRayState rayState)
{
    SpectralSamples retVal;
    retVal.set(0);

    uint lightNodeIndex = lightVertexIndex;
    LightPathNode lightNode = lightNodeData.node;
                    
    SpectralSamples weightLastCameraNode;
    float pdfForwardLastCameraNode;
    float pdfBackwardLastCameraNode;
	//evaluate weight PDFs for connecting camera node
	{
						
        float pdfForward;
        float pdfBackward;
        evaluateSurfaceAndPDFs(camVertex.surfaceDef, camVertex.precalculatedSurfaceData, camVertex.samplingProbabilities, rayState, camVertex.woOS, camVertex.wiOS, camVertex.triangleHitFrontFace, false, false, weightLastCameraNode, pdfForward, pdfBackward);

        if (pdfForward == 0 || weightLastCameraNode.allSamplesEqual(0))
        {
            return retVal;
        }

        pdfForwardLastCameraNode = pdfForward;
        pdfBackwardLastCameraNode = pdfBackward;
    }
					
    SpectralSamples weightLastLightNode = (SpectralSamples) 0;
    float pdfForwardLastLightNode;
    float pdfBackwardLastLightNode;
    float pdfForwardLastLightNodeMIS;
	//evaluate weight PDFs for connecting light node
	{
        bool connectionValid = calculateConnectingLightNodeWeightsAndPDFs(lightNodeData, lightNodeIndex, lightVertex.prevNodePosWS, camVertex.posWS, pdfForwardLastLightNode, pdfBackwardLastLightNode, weightLastLightNode, pdfForwardLastLightNodeMIS);
        if (!connectionValid)
        {
            return retVal;
        }

    }
    
    float3 toLightWS = lightNode.positionWS.xyz - camVertex.posWS;
    float toLightLenSqr = dot(toLightWS, toLightWS);
    float toLightLen = sqrt(toLightLenSqr);
    float3 toLightDir = toLightWS / toLightLen;
	//trace visibility ray to check visibility with node
	{
        const float VISIBILITY_RAY_EPSILON = 0.001f;
        
		
        uint rayFlags = RAY_FLAG_NONE;
        uint InstanceInclusionMask = ~0;
        RayDesc ray;
        ray.Origin = camVertex.posWS + getRaySpawnOffsetTowardsRay(toLightDir);
        ray.Direction = toLightDir;
        ray.TMin = DEFAULT_RAY_MIN_T;
        ray.TMax = DEFAULT_RAY_MIN_T + toLightLen + VISIBILITY_RAY_EPSILON;

        RayQuery < RAY_FLAG_NONE > q;
        q.TraceRayInline(
							g_accelerationStructure,
							rayFlags,
							InstanceInclusionMask,
							ray);

        while (q.Proceed());
						
        bool lightNodeVisible = false;
        bool isEnvLight = (lightNode.flags & LIGHT_PATH_NODE_FLAG_ENV_LIGHT) != 0;
						
        if (q.CommittedStatus() == COMMITTED_TRIANGLE_HIT)
        {
            uint visInstId = q.CommittedInstanceID();
            uint visPrimIndex = q.CommittedPrimitiveIndex();
							
			//if we hit the light node instance and primitive, good enough for now (could check barycentrics to make sure but should be neglible)
            if (visInstId == lightNode.instanceIndex && visPrimIndex == lightNode.primitiveIndex)
            {
                lightNodeVisible = true;
            }

        }
        else if (isEnvLight)
        {
            lightNodeVisible = true;
        }
						
        if (!lightNodeVisible)
        {
            return retVal;
        }
    }
					
	//can connect, evaluate connection and accumulate total light
	{
        float misWeight = calculateMISWeight(cameraVertexIndex, camVertex, lightVertexIndex, lightVertex, lightNode, pdfBackwardLastCameraNode, pdfForwardLastCameraNode, pdfBackwardLastLightNode, pdfForwardLastLightNodeMIS);

        SpectralSamples radianceFromLight;
        radianceFromLight.fromFloat4(lightNode.radiance);
		
        float geometryTerm = 1.f;

        if (lightVertexIndex > 0)
        {
            geometryTerm = 1.f / toLightLenSqr;
			//the solid angle projection of dot(wi,shadingNormal) is already included in surface eval
			//geometryTerm *= abs(dot(camVertex.normalWS, toLightDir));
			//geometryTerm *= abs(dot(lightNode.normalWS, toLightDir));
        }

        SpectralSamples absorptionOnConnection = rayState.getAbsorption();
        if (!absorptionOnConnection.allSamplesEqual(0))
        {
            weightLastCameraNode = weightLastCameraNode * calculateTransmittance(toLightLen, absorptionOnConnection);
        }

        retVal = weightLastLightNode * weightLastCameraNode * camVertex.throughput * radianceFromLight * geometryTerm * misWeight;
		
		
       

        return retVal;
    }
	
}

bool shouldPathContribute(uint cameraNodeIndex, uint lightNodeIndex)
{
    bool skipCameraNode = (cameraNodeIndex < g_effectiveCameraPathNodesRange.x) || (cameraNodeIndex > g_effectiveCameraPathNodesRange.y);
    bool skipLightNode = (lightNodeIndex < g_effectiveLightPathNodesRange.x) || (lightNodeIndex > g_effectiveLightPathNodesRange.y);

    return !skipCameraNode && !skipLightNode;
}


#endif