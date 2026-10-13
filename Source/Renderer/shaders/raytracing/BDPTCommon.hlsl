#ifndef BDPT_COMMON_HLSL
#define BDPT_COMMON_HLSL

static float g_spectralMainSampleWavelength;
#define GET_SPECTRAL_SAMPLE_WAVELENGTH g_spectralMainSampleWavelength;

#include "../globalDefinitions.hlsl"
#include "hitShadersCommon.hlsl"
#include "materialSample.hlsl"
#include "rayState.hlsl"
#include "lightSampling.hlsl"

//set when the radiance stored in the node has already had its secondary wavelengths terminated by a dispersive event earlier on the light subpath
#define LIGHT_PATH_NODE_FLAG_SECONDARY_WAVELENGTHS_TERMINATED (1 << 0)
#define LIGHT_PATH_NODE_FLAG_HIT_FRONT_FACE (1 << 1)
#define LIGHT_PATH_NODE_FLAG_ENV_LIGHT (1 << 2)
#define LIGHT_PATH_NODE_FLAG_IS_DELTA_DISTRIBUTION (1 << 3)
//set if the path has already been regularized
#define LIGHT_PATH_NODE_FLAG_REGULARIZED (1 << 4)
//the node is a subsurface exit point: the path arrived through the subsurface from the previous node (no ray between them), so only the exit lobe applies here
#define LIGHT_PATH_NODE_FLAG_SUBSURFACE_EXIT (1 << 5)

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

    int findVolume(uint materialIndex)
    {
        for (int i = int(numberVolumesEntered) - 1; i >= 0; --i)
        {
            if (volumeMaterial[i] == materialIndex)
            {
                return i;
            }
        }
        return -1;
    }

    void enteredVolume(float IOR, SpectralSamples absorptionParam, uint materialIndex)
    {
        numberVolumesEntered = min(numberVolumesEntered + 1, RAY_MAX_VOLUMES_ENTERED);
        ior[numberVolumesEntered - 1] = IOR;
        absorption[numberVolumesEntered - 1] = absorptionParam;
        volumeMaterial[numberVolumesEntered - 1] = materialIndex;
    }


    void exitedVolume(uint materialIndex)
    {
        if (numberVolumesEntered == 0)
        {
            return;
        }
        int index = findVolume(materialIndex);
        if (index < 0)
        {
            index = int(numberVolumesEntered) - 1;
        }
        for (uint i = uint(index); i + 1 < numberVolumesEntered; ++i)
        {
            ior[i] = ior[i + 1];
            absorption[i] = absorption[i + 1];
            volumeMaterial[i] = volumeMaterial[i + 1];
        }
        --numberVolumesEntered;
    }

    uint getNumberOfVolumesEntered()
    {
        return numberVolumesEntered;
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
    uint4 volumeMaterial;
    uint numberVolumesEntered;
    uint flags;
};



ConstantBuffer<BidirectionalPathTraceConstants> g_bdptConstants : register(b0, space3);

RaytracingAccelerationStructure g_accelerationStructure : register(t1, space3);

#include "scatteringSample.hlsl"

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

//2 for init, 1 for wavelength, 8 for light sample, 5 per vertex
#define g_cameraPathRandomDimensionOffset (g_maxVerticesPerLightPath * 5 + 12)
//lens sample used when connecting pure light paths to the camera, placed in the unused gap between light path and camera path dimensions
#define g_pureLightPathLensRandomDimensionOffset (g_maxVerticesPerLightPath * 5 + 9)
#define g_cameraRayWorkGroupCount (g_bdptConstants.cameraRayWorkGroupCount)
#define g_cameraWorkGroupOffset (g_bdptConstants.cameraWorkGroupSwizzleOffset)

#define g_sampleEnvLightProbabilityWeight 0.0f
#define g_effectiveCameraPathNodesRange (g_bdptConstants.lightAndCameraPathConstraints.zw)
#define g_effectiveLightPathNodesRange (g_bdptConstants.lightAndCameraPathConstraints.xy)

#define THREAD_GROUP_DIM_SIZE_CAM_RAY 8

//storage layout shared by the pure light path kernels (initialize/calculate/accumulate)
#define INVALID_PURE_LIGHT_NODE_INDEX (0xFFFFFFFFu)

struct PureLightPathNodeHeader
{
    uint first;
    uint latest;
};

uint calculatePureLightPathHeaderOffset(uint2 texel)
{
    uint offset = texel.y * g_targetTexDimensions.x + texel.x;
    offset *= 2 << 2;
    return offset;
}

uint getPureLightPathNodeSizeInBytes()
{
    return 4 << 2;
}

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

struct ScatteringDirections
{
    float3 woOS;
    float3 wiOS;
    bool triangleHitFrontFace;
    bool isFromLightSource;
    bool treatAsDelta;
};

ScatteringDirections makeScatteringDirections(float3 woOS, float3 wiOS, bool triangleHitFrontFace, bool isFromLightSource, bool treatAsDelta)
{
    ScatteringDirections dirs;
    dirs.woOS = woOS;
    dirs.wiOS = wiOS;
    dirs.triangleHitFrontFace = triangleHitFrontFace;
    dirs.isFromLightSource = isFromLightSource;
    dirs.treatAsDelta = treatAsDelta;
    return dirs;
}

ScatteringEvaluation evaluateSurfaceAndPDFs(in SurfaceSamplingContext surface, inout BDPTRayState rayState, in uint materialIndex, in ScatteringDirections dirs)
{
    ScatteringEvaluation result;
    result.dispersed = false;

    SurfaceDefinition surfaceDef = surface.surfaceDef;

    uint flags = EVALUATE_FLAGS_NONE;
    if (dirs.isFromLightSource)
    {
        flags |= EVALUATE_FLAGS_LIGHT_PATH;
    }
    if (dirs.treatAsDelta)
    {
        flags |= EVALUATE_FLAGS_TREAT_AS_DELTA;
    }

    SurfaceEvaluation forward = evaluateSurface(surface, dirs.woOS, dirs.wiOS, flags);
    result.weight = forward.weight;
    result.pdfForward = forward.pdf;
    TransmissionType transmissionType = forward.transmissionType;

    //This is silly but surface data is not the same when flippiing wo/wi so have to recalculate it here. TODO: recalculate only things that are independant of the direction and calculate wo/wi dependant things only later on
    {
        bool triangleHitFrontfaceReverseDir = dirs.triangleHitFrontFace;
        bool transmittedPair = dot(dirs.woOS, surfaceDef.geometryNormal) * dot(dirs.wiOS, surfaceDef.geometryNormal) < 0;
        if (transmittedPair)
        {
            triangleHitFrontfaceReverseDir = !triangleHitFrontfaceReverseDir;

        }

        SurfaceSamplingContext reverseSurface;
        reverseSurface.surfaceDef = surfaceDef;
        getPrecalculatedSurfaceData(surfaceDef, rayState.getCurrentIOR(), rayState.getPreviousIOR(), dirs.wiOS, triangleHitFrontfaceReverseDir, reverseSurface.precalculatedSurfaceData);

        if (transmittedPair)
        {
            reverseSurface.precalculatedSurfaceData.fromIOR = surface.precalculatedSurfaceData.toIOR;
            reverseSurface.precalculatedSurfaceData.toIOR = surface.precalculatedSurfaceData.fromIOR;
        }

        calculateNormalizedMaterialLayerSamplingProbabilities(surfaceDef, reverseSurface.precalculatedSurfaceData, reverseSurface.samplingProbabilities);
        result.pdfBackward = evaluateSurface(reverseSurface, dirs.wiOS, dirs.woOS, EVALUATE_FLAGS_PDF_ONLY).pdf; //generate pdf for reversed order
    }


    if (transmissionType != TRANSMISSION_TYPE_NONE)
    {
        if ((transmissionType & TRANSMISSION_TYPE_DISPERSED) != 0)
        {
            result.dispersed = (rayState.getStateFlags() & RAYSTATE_FLAGS_SECONDARY_LAMBDAS_TERMINATED) == 0;
            rayState.setStateFlags(rayState.getStateFlags() | RAYSTATE_FLAGS_SECONDARY_LAMBDAS_TERMINATED);
        }

        if ((transmissionType & TRANSMISSION_TYPE_EXITED) != 0)
        {
            rayState.exitedVolume(materialIndex);

        }
        else if ((transmissionType & TRANSMISSION_TYPE_ENTERED) != 0)
        {
            rayState.enteredVolume(surfaceDef.dielectricIOR, surfaceDef.absorption, materialIndex);
        }
    }
    return result;
}

struct SubsurfaceEdge
{
    float3 exitPosWS;
    float3 exitNormalWS;
    float3 entryPosWS;
    float3 entryNormalWS;
};

SubsurfaceEdge makeSubsurfaceEdge(float3 exitPosWS, float3 exitNormalWS, float3 entryPosWS, float3 entryNormalWS)
{
    SubsurfaceEdge edge;
    edge.exitPosWS = exitPosWS;
    edge.exitNormalWS = exitNormalWS;
    edge.entryPosWS = entryPosWS;
    edge.entryNormalWS = entryNormalWS;
    return edge;
}

ScatteringEvaluation evaluateSubsurfaceExitAndPDFs(in SurfaceSamplingContext surface, in BDPTRayState rayState, in ScatteringDirections dirs, in SubsurfaceEdge edge)
{
    ScatteringEvaluation result;
    result.dispersed = false;
    evaluateSubsurfaceExitLobe(surface.surfaceDef, surface.precalculatedSurfaceData, dirs.woOS, dirs.wiOS, dirs.isFromLightSource ? EVALUATE_FLAGS_LIGHT_PATH : EVALUATE_FLAGS_NONE, result.weight, result.pdfForward);

    SurfaceSamplingContext reverseSurface = makeSurfaceSamplingContext(surface.surfaceDef, rayState.getCurrentIOR(), rayState.getPreviousIOR(), dirs.wiOS, dirs.triangleHitFrontFace);
    result.pdfBackward = reverseSurface.samplingProbabilities[LAYERIND_SUBSURFACE] * pdfSubsurfaceExit(surface.surfaceDef, edge.exitPosWS, edge.exitNormalWS, edge.entryPosWS, edge.entryNormalWS);
    return result;
}

//a path vertex, either found by a ray or the exit point of a subsurface event (in which case there is no ray, and wo is the outward normal)
struct PathHit
{
    float3x4 worldToObject;
    float3 posWS;
    uint instanceIndex;
    float3 woOS;
    uint primitiveIndex;
    float3 woWS;
    float rayT;
    float2 barycentrics;
    bool frontFace;
    bool isSubsurfaceExit;
};

PathHit makePathHit(float3 rayOriginWS, float3 rayDirWS, float rayT, uint instanceIndex, uint primitiveIndex, float2 barycentrics, bool frontFace, float3 objectRayDir, float3x4 worldToObject)
{
    PathHit hit;
    hit.worldToObject = worldToObject;
    hit.posWS = rayOriginWS + rayDirWS * rayT;
    hit.instanceIndex = instanceIndex;
    hit.woOS = normalize(-objectRayDir);
    hit.primitiveIndex = primitiveIndex;
    hit.woWS = normalize(-rayDirWS);
    hit.rayT = rayT;
    hit.barycentrics = barycentrics;
    hit.frontFace = frontFace;
    hit.isSubsurfaceExit = false;
    return hit;
}

#define MAKE_PATH_HIT_FROM_RAY_QUERY(q) makePathHit(q.WorldRayOrigin(), q.WorldRayDirection(), q.CommittedRayT(), q.CommittedInstanceID(), q.CommittedPrimitiveIndex(), q.CommittedTriangleBarycentrics(), q.CommittedTriangleFrontFace(), q.CommittedObjectRayDirection(), q.CommittedWorldToObject3x4())

PathHit makePathHitFromSubsurfaceExit(SurfacePoint exitPoint)
{
    PathHit hit;
    hit.worldToObject = getTransformDataForInstance(exitPoint.instanceIndex).getWorldToObj();
    hit.posWS = exitPoint.positionWS;
    hit.instanceIndex = exitPoint.instanceIndex;
    hit.woOS = normalize(mul((float3x3) hit.worldToObject, exitPoint.geometryNormalWS));
    hit.primitiveIndex = exitPoint.primitiveIndex;
    hit.woWS = exitPoint.geometryNormalWS;
    hit.rayT = 0;
    hit.barycentrics = exitPoint.barycentrics;
    hit.frontFace = exitPoint.frontFace;
    hit.isSubsurfaceExit = true;
    return hit;
}

SurfacePoint makeSurfacePoint(PathHit hit, float3 geometryNormalWS)
{
    SurfacePoint p;
    p.positionWS = hit.posWS;
    p.instanceIndex = hit.instanceIndex;
    p.geometryNormalWS = orientToSameSide(hit.woWS, geometryNormalWS);
    p.primitiveIndex = hit.primitiveIndex;
    p.barycentrics = hit.barycentrics;
    p.frontFace = hit.frontFace;
    return p;
}


float calculateCameraDirectionalPDF(float3 camToNodeWS)
{
	float4 planeMin = mul(g_uvToViewTransform, float4(0, 0, 0, 1));
	float4 planeMax = mul(g_uvToViewTransform, float4(1, 1, 0, 1));
	planeMin /= planeMin.w;
	planeMax /= planeMax.w;

	float2 planeExtent = abs(planeMax.xy - planeMin.xy);
	float planeArea = safeDiv(planeExtent.x * planeExtent.y, planeMin.z * planeMin.z);

	float3 camToNodeVS = mul(g_worldToViewTransform, float4(camToNodeWS, 0)).xyz;
	float cosTheta = abs(camToNodeVS.z) * rsqrt(max(0.0001f, dot(camToNodeVS, camToNodeVS)));
    //omit one costheta because it would cancel with cameras geometry term costheta (and the lightnode already has one from Surface eval)
	return safeDiv(g_targetTexDimensions.x * g_targetTexDimensions.y, planeArea * cosTheta * cosTheta * cosTheta);  
}


#define MAX_PDF_RATIO_SUM 1e16f

float calculatePDFRatio(float nom, float denom)
{
    nom = nom < 0 ? 1.f : nom;
    denom = denom < 0 ? 1.f : denom;
    return min(safeDiv(nom, denom), MAX_PDF_RATIO_SUM);
}

float accumulateRISum(float riSum, float nom, float denom, bool connectable)
{
    float ri = calculatePDFRatio(nom, denom);
    bool accumulateRI = connectable && nom > 0 && denom > 0;
    riSum = ri * riSum;
    if(accumulateRI)
    {
        riSum += ri;
    }
    return min(riSum, MAX_PDF_RATIO_SUM);
}

float accumulateRISum(float riSum, float nom, float denom)
{
    return accumulateRISum(riSum, nom, denom, true);
}

ExtractedLightPathNodeData getExtractedLightPathNodeData(LightPathNode node)
{
    ExtractedLightPathNodeData data;
    data.node = node;
    fillSurfaceDefRGB(node.instanceIndex, node.primitiveIndex, node.barycentrics, data.surfaceDefRGB);
    data.worldToObjSpace = getTransformDataForInstance(node.instanceIndex).getWorldToObj();
    return data;
}

SurfaceSamplingContext calculateCommonSurfaceParams(inout BDPTRayState rayState, in uint materialIndex, SurfaceDefinitionRGB surfaceDefRGB, in float3 woOS, bool triangleHitFrontFace)
{

    handleTwoSidedMaterialOrientation(surfaceDefRGB, woOS);

    if ((rayState.getStateFlags() & RAYSTATE_FLAGS_REGULARIZE_PATH) != 0)
    {
        regularizeMaterial(surfaceDefRGB);
    }

    SurfaceDefinition surfaceDef = convertSurfaceDefinitionFromRGB(surfaceDefRGB);

    //must happen before the precalculated data is set up so that the IORs (and the absorption applied by the caller) account for the medium the ray started in
    handleRayStartInsideMedium(rayState, surfaceDef, materialIndex, -woOS, surfaceDef.geometryNormal);

    return makeSurfaceSamplingContext(surfaceDef, rayState.getCurrentIOR(), rayState.getPreviousIOR(), woOS, triangleHitFrontFace);
}


SurfaceSamplingContext calculateCommonSurfaceParams(inout BDPTRayState rayState, in uint instanceIndex, in uint primitiveIndex, in float2 barycentrics2, in float3 woOS, bool triangleHitFrontFace)
{
    SurfaceDefinitionRGB surfaceDefRGB;
    fillSurfaceDefRGB(instanceIndex, primitiveIndex, barycentrics2, surfaceDefRGB);

    return calculateCommonSurfaceParams(rayState, getMaterialAndMeshIndicesForInstance(instanceIndex).x, surfaceDefRGB, woOS, triangleHitFrontFace);
}


void pdfForSamplingLightNode(LightPathNode lightNode, float3 towardsDir, float envSampleRelativeProbability, out float lightPickPDF, out float posPDF, out float dirPDF)
{
    pdfForSamplingLight(lightNode.instanceIndex, lightNode.primitiveIndex, lightNode.barycentrics, lightNode.normalWS, towardsDir, envSampleRelativeProbability, lightPickPDF, posPDF, dirPDF);
}

struct LightOrEnvSample
{
    LightSampleOutput sample;
    float pdfLightSelection;
    bool sampledEnvironment;
};

LightOrEnvSample sampleEnvironmentAsLight(float4 lightSampleRand0, float pdfLightSelection)
{
    LightOrEnvSample result = (LightOrEnvSample)0;
    sampleEnvironmentLighting(lightSampleRand0, result.sample.radiance, result.sample.positionWS, result.sample.directionWS, result.sample.pdfPos, result.sample.pdfDir);
    result.sample.normalWS = 0;
    result.sample.instanceIndex = -1;
    result.sample.primitiveIndex = -1;
    result.sample.emissionProfileWeight = 1.f;
    result.pdfLightSelection = pdfLightSelection;
    result.sampledEnvironment = true;
    return result;
}

LightOrEnvSample sampleLightOrEnv(float lightPickRand, float4 lightSampleRand0, float2 lightSampleRand1, float envSampleRelativeProbability)
{
    if (g_lightCount == 0)
    {
        return sampleEnvironmentAsLight(lightSampleRand0, 1.f);
    }

    float lightProb = g_lightCount + envSampleRelativeProbability;
    float s = lightPickRand * lightProb;
    if (s > g_lightCount)
    {
        return sampleEnvironmentAsLight(lightSampleRand0, envSampleRelativeProbability / lightProb);
    }

    uint lightIndex = min((uint) floor(lightPickRand * g_lightCount), g_lightCount - 1);
    LightOrEnvSample result;
    sampleLight(lightIndex, lightSampleRand0.xyz, lightSampleRand1, result.sample);
    result.pdfLightSelection = 1 / lightProb;
    result.sampledEnvironment = false;
    return result;
}


//for a subsurface exit node evalOut.pdfBackward is an area density (see evaluateSubsurfaceExitAndPDFs)
bool calculateConnectingLightNodeWeightsAndPDFs(ExtractedLightPathNodeData lightNodeData, uint lightNodeIndex, float3 prevVertexPos, float3 prevVertexNormal, float3 nextVertexPos, bool secondaryWavelengthsAlreadyTerminated, out ScatteringEvaluation evalOut, out float pdfForwardMISOut)
{
    evalOut = (ScatteringEvaluation)0;

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
            pdfForwardMISOut = 0;
            return false;
        }

		//for first node, this is just pdf for selecting light, ie 1.f/probLightPick*probPos
        float pdf = lightNode.pdfForwardMIS;
        evalOut.pdfForward = pdf * areaDensityToSolidAngleMultiplier(-fromLightWS, lightNode.normalWS);

        float cosLightEmit = dot(lightNode.normalWS, normalize(fromLightWS));
        float emissionFocus = ((lightNodeFlags & LIGHT_PATH_NODE_FLAG_ENV_LIGHT) != 0) ? 0.f : surfaceDefRGB.emissionFocus;
        evalOut.weight.set(safeDiv(evaluateEmissionProfile(cosLightEmit, emissionFocus), evalOut.pdfForward));

        pdfForwardMISOut = twoSided ? pdfCosinePowerWeightedSphere(cosLightEmit, emissionFocus) : (cosLightEmit >= 0 ? pdfCosinePowerWeightedHemisphere(cosLightEmit, emissionFocus) : 0);


    }
    else if ((lightNodeFlags & LIGHT_PATH_NODE_FLAG_SUBSURFACE_EXIT) != 0)
    {
        float3 lightPosOS = mul(toLocalFrame, float4(lightNode.positionWS.xyz, 1)).xyz;
        float3 camNodePosOS = mul(toLocalFrame, float4(nextVertexPos, 1)).xyz;
        float3 wiOS = normalize(camNodePosOS - lightPosOS);
        //exit nodes store the outward normal
        float3 woOS = normalize(mul((float3x3) toLocalFrame, lightNode.normalWS));

        handleTwoSidedMaterialOrientation(surfaceDefRGB, woOS);
        if ((lightNodeFlags & LIGHT_PATH_NODE_FLAG_REGULARIZED) != 0)
        {
            regularizeMaterial(surfaceDefRGB);
        }

        BDPTRayState rayState;
        rayState.ior[0] = lightNode.iorPrevious;
        rayState.ior[1] = lightNode.iorCurrent;
        rayState.volumeMaterial = VOLUME_MATERIAL_NONE;
        rayState.numberVolumesEntered = 2;
        rayState.setStateFlags(0);

        SurfaceSamplingContext surface = makeSurfaceSamplingContext(convertSurfaceDefinitionFromRGB(surfaceDefRGB), rayState.getCurrentIOR(), rayState.getPreviousIOR(), woOS, triangleHitFrontFace);
        evalOut = evaluateSubsurfaceExitAndPDFs(surface, rayState, makeScatteringDirections(woOS, wiOS, triangleHitFrontFace, true, false),
            makeSubsurfaceEdge(lightNode.positionWS, lightNode.normalWS, prevVertexPos, prevVertexNormal));

        pdfForwardMISOut = evalOut.pdfForward;

        if (evalOut.pdfBackward == 0 || evalOut.weight.allSamplesEqual(0))
        {
            return false;
        }
    }
    else
    {

        float3 lightPosOS = mul(toLocalFrame, float4(lightNode.positionWS.xyz, 1)).xyz;
        float3 prevLightPosOS = mul(toLocalFrame, float4(prevVertexPos, 1)).xyz;
        float3 camNodePosOS = mul(toLocalFrame, float4(nextVertexPos, 1)).xyz;
        float3 wiOS = normalize(camNodePosOS - lightPosOS);
        float3 woOSLightConnection = normalize(prevLightPosOS - lightPosOS);


		//match the regularization the light subpath was traced with, so this evaluation and the pdfs stored on the node agree
        if ((lightNodeFlags & LIGHT_PATH_NODE_FLAG_REGULARIZED) != 0)
        {
            regularizeMaterial(surfaceDefRGB);
        }


//for now just initialize with node data. maybe needs a better way (update during light node traversal?)
        BDPTRayState rayState;
        rayState.ior[0] = lightNode.iorPrevious;
        rayState.ior[1] = lightNode.iorCurrent;
        rayState.volumeMaterial = VOLUME_MATERIAL_NONE;
        rayState.numberVolumesEntered = 2;

        bool alreadyTerminated = secondaryWavelengthsAlreadyTerminated || ((lightNodeFlags & LIGHT_PATH_NODE_FLAG_SECONDARY_WAVELENGTHS_TERMINATED) != 0);
        rayState.setStateFlags(alreadyTerminated ? RAYSTATE_FLAGS_SECONDARY_LAMBDAS_TERMINATED : 0);

        SurfaceSamplingContext surface = makeSurfaceSamplingContext(convertSurfaceDefinitionFromRGB(surfaceDefRGB), rayState.getCurrentIOR(), rayState.getPreviousIOR(), woOSLightConnection, triangleHitFrontFace);
        evalOut = evaluateSurfaceAndPDFs(surface, rayState, VOLUME_MATERIAL_NONE, makeScatteringDirections(woOSLightConnection, wiOS, triangleHitFrontFace, true, false));

        if (evalOut.dispersed)
        {
            evalOut.weight.terminateSecondaryWavelengths();
        }

        pdfForwardMISOut = evalOut.pdfForward;

        if (evalOut.pdfBackward == 0 || evalOut.weight.allSamplesEqual(0))
        {
            return false;
        }
    }

    return true;
}

struct CameraVertexContext
{
    SurfaceSamplingContext surface;
    SpectralSamples throughput;
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
    uint materialIndex;
    bool isSubsurfaceExit; //reached through the subsurface from the previous vertex
    bool prevIsSubsurfaceExit;
};

struct LightVertexContext
{
    float3 prevNodePosWS;
    float previousPDFForwardMIS;
    float3 prevNodeNormalWS;
    uint originatingNodeFlags;
    float riSum;
    uint prevNodeFlags;
};


//pdfs of the vertices on both ends of the connection, evaluated for the connected path
struct ConnectionPDFs
{
    float pdfForwardLastCameraNode;
    float pdfBackwardLastCameraNode;
    float pdfForwardLastLightNode;
    float pdfBackwardLastLightNode;
};

float calculateMISWeight(uint cameraVertexIndex, in CameraVertexContext camVertex,
uint lightVertexIndex, in LightVertexContext lightVertex, in LightPathNode lightNode, in ConnectionPDFs pdfs)
{

    float riSumCam = camVertex.pdfRatioSum;
	//TODO: the actual first node would be the origina of the ray from camera but its not considered (and can never be hit by path from light). Revisit if its correct to just ignore it here
    if (cameraVertexIndex > 1)
    {
        //from a subsurface exit the backward pdf is already an area density
        float pdfBackwardPrevNodeCam = camVertex.isSubsurfaceExit ? pdfs.pdfBackwardLastCameraNode : pdfs.pdfBackwardLastCameraNode * solidAngleToAreaDensityMultiplier(camVertex.prevNodePosWS - camVertex.posWS, camVertex.prevNodeNormalWS);
        riSumCam = accumulateRISum(riSumCam, pdfBackwardPrevNodeCam, camVertex.previousPDFForwardMIS, !camVertex.prevIsSubsurfaceExit);
    }
    if (cameraVertexIndex > 0)
    {
        riSumCam = accumulateRISum(riSumCam, pdfs.pdfForwardLastLightNode * solidAngleToAreaDensityMultiplier(camVertex.posWS - lightNode.positionWS.xyz, camVertex.normalWS), camVertex.currentNodeForwardPDFMIS, !camVertex.isSubsurfaceExit);
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
            misWeightToLight = pdfs.pdfForwardLastCameraNode;
        }
        else
        {
            pdfForSamplingLightNode(lightNode, lightToCam, g_sampleEnvLightProbabilityWeight, lightPickPDF, lightPosPDF, lightDirPDF);
            misWeightToLight = pdfs.pdfForwardLastCameraNode * solidAngleToAreaDensityMultiplier(camToLight, lightNode.normalWS);
        }
		
        riSumLight = calculatePDFRatio(misWeightToLight, (lightPickPDF * lightPosPDF));
		
    }
    else
    {
        float pdfBackwardPrevNodeLight = pdfs.pdfBackwardLastLightNode;

        bool lightNodeIsSubsurfaceExit = (lightNode.flags & LIGHT_PATH_NODE_FLAG_SUBSURFACE_EXIT) != 0;
        bool prevNodeIsSubsurfaceExit = (lightVertex.prevNodeFlags & LIGHT_PATH_NODE_FLAG_SUBSURFACE_EXIT) != 0;
        bool prevNodeIsEnvLight = (lightVertexIndex == 1) && ((lightVertex.originatingNodeFlags & LIGHT_PATH_NODE_FLAG_ENV_LIGHT) != 0);
        if (!prevNodeIsEnvLight && !lightNodeIsSubsurfaceExit)
        {
            pdfBackwardPrevNodeLight *= solidAngleToAreaDensityMultiplier(lightVertex.prevNodePosWS - lightNode.positionWS, lightVertex.prevNodeNormalWS);
        }

        riSumLight = accumulateRISum(riSumLight, pdfBackwardPrevNodeLight, lightVertex.previousPDFForwardMIS, !prevNodeIsSubsurfaceExit);
        riSumLight = accumulateRISum(riSumLight, pdfs.pdfForwardLastCameraNode * solidAngleToAreaDensityMultiplier(lightNode.positionWS.xyz - camVertex.posWS, lightNode.normalWS), lightNode.pdfForwardMIS, !lightNodeIsSubsurfaceExit);
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
        riSumCam = accumulateRISum(riSumCam, lightDirPDF * solidAngleToAreaDensityMultiplier(towardsPreviousCameraNode, camVertexContext.prevNodeNormalWS), camVertexContext.previousPDFForwardMIS, !camVertexContext.prevIsSubsurfaceExit);
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
        riSumCam = accumulateRISum(riSumCam, lightDirPDF * solidAngleToAreaDensityMultiplier(towardsPreviousCameraNode, context.prevNodeNormalWS), context.previousPDFForwardMIS, !context.prevIsSubsurfaceExit);
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

	//the connected path is single wavelength as soon as either subpath has dispersed, and only the first dispersive event along it
	//rescales the hero sample.
    bool cameraSubpathTerminated = rayState.hasFlags(RAYSTATE_FLAGS_SECONDARY_LAMBDAS_TERMINATED);
    bool lightSubpathTerminated = (lightNode.flags & LIGHT_PATH_NODE_FLAG_SECONDARY_WAVELENGTHS_TERMINATED) != 0;
    if (lightSubpathTerminated)
    {
        rayState.addFlags(RAYSTATE_FLAGS_SECONDARY_LAMBDAS_TERMINATED);
    }

    ConnectionPDFs connectionPDFs;
    SpectralSamples weightLastCameraNode;
	//evaluate weight PDFs for connecting camera node
	{
        ScatteringDirections dirs = makeScatteringDirections(camVertex.woOS, camVertex.wiOS, camVertex.triangleHitFrontFace, false, false);
        ScatteringEvaluation eval;
        if (camVertex.isSubsurfaceExit)
        {
            eval = evaluateSubsurfaceExitAndPDFs(camVertex.surface, rayState, dirs, makeSubsurfaceEdge(camVertex.posWS, camVertex.normalWS, camVertex.prevNodePosWS, camVertex.prevNodeNormalWS));
        }
        else
        {
            eval = evaluateSurfaceAndPDFs(camVertex.surface, rayState, camVertex.materialIndex, dirs);
        }

        if (eval.dispersed)
        {
            eval.weight.terminateSecondaryWavelengths();
        }

        if (eval.pdfForward == 0 || eval.weight.allSamplesEqual(0))
        {
            return retVal;
        }

        weightLastCameraNode = eval.weight;
        connectionPDFs.pdfForwardLastCameraNode = eval.pdfForward;
        connectionPDFs.pdfBackwardLastCameraNode = eval.pdfBackward;
    }

    SpectralSamples weightLastLightNode;
	//evaluate weight PDFs for connecting light node
	{
        ScatteringEvaluation eval;
        bool connectionValid = calculateConnectingLightNodeWeightsAndPDFs(lightNodeData, lightNodeIndex, lightVertex.prevNodePosWS, lightVertex.prevNodeNormalWS, camVertex.posWS, rayState.hasFlags(RAYSTATE_FLAGS_SECONDARY_LAMBDAS_TERMINATED), eval, connectionPDFs.pdfForwardLastLightNode);
        if (!connectionValid)
        {
            return retVal;
        }
        weightLastLightNode = eval.weight;
        connectionPDFs.pdfBackwardLastLightNode = eval.pdfBackward;
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
        ray.Origin = spawnRayOrigin(camVertex.posWS, camVertex.normalWS, toLightDir);
        ray.Direction = toLightDir;
        ray.TMin = DEFAULT_RAY_MIN_T;
        ray.TMax = DEFAULT_RAY_MIN_T + toLightLen + VISIBILITY_RAY_EPSILON;

        RayQuery < RAY_FLAG_NONE > q;
        q.TraceRayInline(
							g_accelerationStructure,
							rayFlags,
							InstanceInclusionMask,
							ray);

        PROCEED_ALPHA_TESTED(q)
						
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
        float misWeight = calculateMISWeight(cameraVertexIndex, camVertex, lightVertexIndex, lightVertex, lightNode, connectionPDFs);

        SpectralSamples radianceFromLight;
        radianceFromLight.fromFloat4(lightNode.radiance);
		
        float geometryTerm = 1.f;

        if (lightVertexIndex > 0)
        {
            geometryTerm = 1.f / toLightLenSqr;
        }

        SpectralSamples absorptionOnConnection = rayState.getAbsorption();
        if (!absorptionOnConnection.allSamplesEqual(0))
        {
            weightLastCameraNode = weightLastCameraNode * calculateTransmittance(toLightLen, absorptionOnConnection);
        }

        retVal = weightLastLightNode * weightLastCameraNode * camVertex.throughput * radianceFromLight * geometryTerm * misWeight;

		//remove duplocate wavelength scale
        if (cameraSubpathTerminated && lightSubpathTerminated)
        {
            retVal.undoSecondaryWavelengthRescale();
        }

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