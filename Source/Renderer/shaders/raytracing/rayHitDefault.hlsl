#include "BPTShared.hlsl"
#include "materialSample.hlsl"
#include "payload.hlsl"
#include "lightSampling.hlsl"


struct RayHitShaderTableConstantData
{
	uint2 materialAndMeshIndices;
};
SHADERTABLE_EXTRADATA_DECLARE(RayHitShaderTableConstantData);

#ifdef ENABLE_NEE

float geometryTerm(float3 fromNormal, float3 toNormal, float3 fromToUnnormalized)
{
    float invDistSqr = 1.f / dot(fromToUnnormalized, fromToUnnormalized);
    float3 fromToNormalized = fromToUnnormalized * sqrt(invDistSqr);
    float absDot = abs(dot(toNormal, fromToNormalized) * dot(fromNormal, fromToNormalized));
    return absDot * invDistSqr;
}

void sampleExplicitLight(in float3 currentPosWS, in float3 normalWS, in float4 lightSampleRand, out float3 lightSamplePositionOut, out SpectralSamples emissionOut, out float pdfOut, out uint instanceIndexOut, out uint primitiveIndexOut)
{
    float3 positionOnLight;
    float3 lightNormalWS;
    float lightSelectionPDF;
    float lightPositionPDF;
    uint instanceIndex;
    uint primitiveIndex;
    float2 baryCentrics;
    sampleRandomLightPosition(lightSampleRand, emissionOut, positionOnLight, lightNormalWS, lightSelectionPDF, lightPositionPDF, instanceIndex, primitiveIndex, baryCentrics);

    float3 toLight = positionOnLight - currentPosWS;
    
    emissionOut = emissionOut * geometryTerm(normalWS, lightNormalWS, toLight);
    pdfOut = lightSelectionPDF * lightPositionPDF; //* areaDensityToSolidAngleMultiplier(toLight, lightNormalWS);
    
    lightSamplePositionOut = positionOnLight;
    primitiveIndexOut = primitiveIndex;
    instanceIndexOut = instanceIndex;

    if(dot(toLight, lightNormalWS) >= 0)
    {
        emissionOut.set(0);
        pdfOut = 0;
    }
}

bool checkLightVisibility(in float3 rayPos, in float3 rayDir, float rayLen, uint lightInstanceID, uint lightPrimitiveIndex)
{
    uint rayFlags = RAY_FLAG_NONE;
    uint InstanceInclusionMask = ~0;
    RayQuery < RAY_FLAG_NONE > q;

    RayDesc ray;
    ray.Origin = rayPos;
    ray.Direction = rayDir;
    ray.TMin = DEFAULT_RAY_MIN_T;
    ray.TMax = rayLen;

    q.TraceRayInline(
		    g_accelerationStructure,
		    rayFlags,
		    InstanceInclusionMask,
		    ray);

    while (q.Proceed())
    {
    }

    if (q.CommittedStatus() == COMMITTED_TRIANGLE_HIT)
    {

        uint instId = q.CommittedInstanceID();
        uint primIndex = q.CommittedPrimitiveIndex();
        return instId == lightInstanceID && primIndex == lightPrimitiveIndex;
    }
    return false;
}

float calculateExplicitLightConnectionPDF(float3 fromPointToLightUnnormalized, in uint instanceIndex, in uint primitiveIndex, float2 bary)
{
    float lightProb = g_lightCount;
    float lightPickPDF = 1.f / lightProb;
    
    uint2 matMeshIndices = getMaterialAndMeshIndicesForInstance(instanceIndex);
    RenderObjectTransformDataGPU transformData = getTransformDataForInstance(instanceIndex);
    MeshEntryGPU meshEntry = getMeshEntry(matMeshIndices.y);
    uint primCount = (meshEntry.indexCount / 3);

    float3 barycentrics = float3(1 - bary.x - bary.y, bary.x, bary.y);
    uint3 indices = fetchIndices(meshEntry.indexBuffer, primitiveIndex);
    float3x4 transf = transformData.getObjToWorld();
    
    float3 p1, p2, p3;
    fetchMeshPositions(meshEntry.positionBuffer, indices, p1, p2, p3);
    p1 = mul(transf, float4(p1, 1)).xyz;
    p2 = mul(transf, float4(p2, 1)).xyz;
    p3 = mul(transf, float4(p3, 1)).xyz;
    float3 geometryNormal = cross(p2 - p1, p3 - p1);
    float geometryNormalLen = length(geometryNormal);
    geometryNormal /= geometryNormalLen;
    float area = geometryNormalLen * 0.5f;

    float posPDF = 1.f / (primCount * area);

    return posPDF * lightPickPDF * areaDensityToSolidAngleMultiplier(fromPointToLightUnnormalized, geometryNormal);
    
}
#endif
//power heuristic
float weightMIS(float a, float b)
{
    float aSqr = a * a;
    float bSqr = b * b;
    return aSqr / (aSqr + bSqr);

}

void evaluateSurfaceAndGenerateNextSampleDirection(in SurfaceDefinition surfaceDef, inout Payload rayState, inout RandomSampler rand, in float3 rayDirObjSpace, in bool triangleHitFrontFace, out SpectralSamples weightOut, out float3 nextSampleDirOut)
{
    float4 randomSamplesBRDF = rand.getRandom4();
	
    PrecalculatedSurfaceData precalculatedSurfData;
    getPrecalculatedSurfaceData(surfaceDef, rayState.getCurrentIOR(), rayState.getPreviousIOR(), -rayDirObjSpace, triangleHitFrontFace, precalculatedSurfData);

    float samplingProbabilities[LAYER_COUNT];
    calculateNormalizedMaterialLayerSamplingProbabilities(surfaceDef, precalculatedSurfData, samplingProbabilities); 
    
	//Next Event Estimation (explicit light connections)
#ifdef ENABLE_NEE
    
    if (g_lightCount > 0)
    {
        float3 currentPosWS = WorldRayOrigin() + WorldRayDirection() * RayTCurrent();
        float3x4 toObjectSpace = WorldToObject3x4();
        
        float4 randomSamplesLight = rand.getRandom4();
        SpectralSamples emission;
        float lightSamplePdf;
        float3 lightSamplePosWS;
        uint lightInstanceIndex;
        uint lightPrimIndex;
   
        float3x3 transformNormal = (float3x3)WorldToObject3x4();
        transformNormal = transpose(transformNormal);
        float3 normalWS = normalize(mul(transformNormal, surfaceDef.geometryNormal));
    
        sampleExplicitLight(currentPosWS, normalWS, randomSamplesLight, lightSamplePosWS, emission, lightSamplePdf, lightInstanceIndex, lightPrimIndex);
    
        if(!emission.allSamplesEqual(0) && lightSamplePdf > 0)
        {
            float3 toLightWS = lightSamplePosWS - currentPosWS;
            float toLightWSLen = length(toLightWS);
            toLightWS /= toLightWSLen; 
    
            float3x3 toOSLight = (float3x3)WorldToObject3x4();
            float3 toLightDirOS = normalize(mul(toOSLight, toLightWS));
            
            SpectralSamples weightSumLight = (SpectralSamples) 0.f;
            float pdfBRDF = 0;
            TransmissionType transmissionTypeDummy;
            evaluateSurface(surfaceDef, -rayDirObjSpace, toLightDirOS, samplingProbabilities, precalculatedSurfData, EVALUATE_FLAGS_NONE, weightSumLight, pdfBRDF, transmissionTypeDummy);
    
            if (!weightSumLight.allSamplesEqual(0))
            {
		    	float3 rayStart = currentPosWS + getRaySpawnOffsetTowardsRay(toLightWS);
                if (checkLightVisibility(rayStart, toLightWS, toLightWSLen + DEFAULT_RAY_MIN_T, lightInstanceIndex, lightPrimIndex))
                {
                    weightSumLight = (weightSumLight / lightSamplePdf) * weightMIS(lightSamplePdf, pdfBRDF);
                    rayState.totalLight = rayState.totalLight + rayState.throughput * weightSumLight * emission;
                }
            }
        }
    }
#endif

	//evaluate next sample direction (BRDF)
    SpectralSamples weightSumBRDF;
    float pdfBRDF;
    float pdfLightDir;
    TransmissionType transmissionType;
    float3 wiObjSpace = getSampleDirectionOS(surfaceDef, precalculatedSurfData, randomSamplesBRDF.w, randomSamplesBRDF.xy, samplingProbabilities);
    if (!isZero(wiObjSpace))
    {
        evaluateSurface(surfaceDef, -rayDirObjSpace, wiObjSpace, samplingProbabilities, precalculatedSurfData, EVALUATE_FLAGS_NONE, weightSumBRDF, pdfBRDF, transmissionType);
    }
	
    rayState.pdfThisRay = pdfBRDF;
    
    if (pdfBRDF > 0.f)
     {
        
		if (transmissionType != TRANSMISSION_TYPE_NONE)
		{
		    if ((transmissionType & TRANSMISSION_TYPE_DISPERSED) != 0)
		    {
		        rayState.setStateFlags(rayState.getStateFlags() | RAYSTATE_FLAGS_SECONDARY_LAMBDAS_TERMINATED);
		    }
			
		    bool entered = (transmissionType & TRANSMISSION_TYPE_ENTERED) != 0;
		    bool exited = (transmissionType & TRANSMISSION_TYPE_EXITED) != 0;

		    if (entered != exited)
		    {
		        if (exited)
		        {
		            rayState.exitedVolume();

		        }
		        else if (entered)
		        {
		            rayState.enteredVolume(surfaceDef.dielectricIOR, surfaceDef.absorption);
		        }
		    }
			
		    
		}

        weightSumBRDF = weightSumBRDF / pdfBRDF;
    }
    //weightSumBRDF.set(0);
    weightOut = weightSumBRDF;
    nextSampleDirOut = wiObjSpace;
}

#ifdef WHITE_FURNACE_TEST
bool handleWhiteFurnaceTest(inout Payload payload)
{
    uint instanceId = InstanceID();
    if(payload.rayIndex == -1)
    {
        payload.rayIndex = instanceId;
    } 
    else
    {
        if (payload.rayIndex != instanceId)
        {
            SpectralSamples s;
            s.setFromRGBUnbounded(float3(1, 1, 1));
            payload.totalLight = payload.totalLight + payload.throughput * s;
            payload.rayState |= RAY_STATE_TERMINATED;
            return false;
        }
    }
   
    return true;
}
#endif
	


[shader("closesthit")]
void rayHitDefault(inout Payload payload, in BuiltInTriangleIntersectionAttributes attr)
{
    
    g_spectralMainSampleWavelength = payload.sampledWavelength;
    
#ifdef WHITE_FURNACE_TEST
    if (!handleWhiteFurnaceTest(payload))
    {
        return;
    }
#endif
    
    
	uint2 materialAndMeshIndices = SHADERTABLE_EXTRADATA.materialAndMeshIndices;
	MeshEntryGPU meshEntry = getMeshEntry(materialAndMeshIndices.y);


	//Initial surface setup
    float3 barycentrics = float3(1 - attr.barycentrics.x - attr.barycentrics.y, attr.barycentrics.x, attr.barycentrics.y);
	uint3 indices = fetchIndices(meshEntry.indexBuffer, PrimitiveIndex());
    float3 geometryNormal = fetchMeshTriangleNormal(meshEntry.positionBuffer, indices, barycentrics);
    float3 normal = fetchMeshNormal(meshEntry.normalBuffer, indices, barycentrics);
	float3 tangent = meshHasValidTangents(meshEntry.tangentBuffer) ? fetchMeshTangent(meshEntry.tangentBuffer, indices, barycentrics) : float3(1.f, 0.f, 0.f);
	float2 uv = meshHasValidUVs(meshEntry.uvBuffer) ? fetchMeshUV(meshEntry.uvBuffer, indices, barycentrics) : float2(0.5f, 0.5f);
	
	
	float3 rayDir = ObjectRayDirection();
	rayDir = normalize(rayDir); //ObjectRayDirection() contains scaling (if present)

	//fetch surface material parameters
	MaterialEntryGPU matEntry = getMaterialEntry(materialAndMeshIndices.x);
	SurfaceDefinitionRGB surfaceDefRGB;
	fetchSurfaceMaterialParameters(matEntry, surfaceDefRGB);

	modifySurfaceMaterialParametersWithTextures(matEntry, uv, normal, tangent, surfaceDefRGB);
    setupSurfaceOrientation(geometryNormal, normal, normal, tangent, surfaceDefRGB);

    writeMaterialParamsForFirstBounce(surfaceDefRGB, g_MaterialParamsOutput0, g_MaterialParamsOutput1);
    
	SurfaceDefinition surfaceDef = convertSurfaceDefinitionFromRGB(surfaceDefRGB);

	//surface setup done, do the rest
	float3 nextSampleDirBRDF;
	float rayDistance = RayTCurrent();
	
	//before handling the intersection, apply and clear absorption
    SpectralSamples absorb = payload.getAbsorption();
    if (!absorb.allSamplesEqual(0))
	{
        payload.throughput = payload.throughput * calculateTransmittance(rayDistance, absorb);
	}
	
	if(!surfaceDef.emissive.allSamplesEqual(0))
    {
        float wMIS = 1.f;
#ifdef ENABLE_NEE
        if (g_lightCount > 0 && payload.pdfThisRay > 0)
        {
            float lightPDF = calculateExplicitLightConnectionPDF(WorldRayDirection() * RayTCurrent(), InstanceID(), PrimitiveIndex(), attr.barycentrics);
            wMIS = weightMIS(payload.pdfThisRay, lightPDF);
        }
		
#endif
		payload.totalLight = payload.totalLight + payload.throughput * surfaceDef.emissive * wMIS;
		payload.rayState = RAY_STATE_TERMINATED;
	}
	else
	{
		
        RandomSampler rand;
        rand.dimensionOffsetAndSeed = payload.randomDimensionOffsetAndScramble;
        
        SpectralSamples w;
        evaluateSurfaceAndGenerateNextSampleDirection(surfaceDef, payload, rand, rayDir, HitKind() == HIT_KIND_TRIANGLE_FRONT_FACE, w, nextSampleDirBRDF);
        payload.throughput = payload.throughput * w;
        
        payload.randomDimensionOffsetAndScramble = rand.dimensionOffsetAndSeed;
        
        if (isZero(nextSampleDirBRDF) || payload.pdfThisRay == 0.0f)
        {
            payload.rayState = RAY_STATE_TERMINATED;
        }
    }

	if(payload.rayState == RAY_STATE_ALIVE)
	{
		float3x3 objToWorldLin =  (float3x3)ObjectToWorld3x4();
	
		float3 normalWorld = mul(objToWorldLin, geometryNormal);
		normalWorld = normalize(normalWorld);

        nextSampleDirBRDF = mul(objToWorldLin, nextSampleDirBRDF);
        nextSampleDirBRDF = normalize(nextSampleDirBRDF);

        payload.rayDirection = nextSampleDirBRDF;
        payload.rayOrigin = WorldRayOrigin() + WorldRayDirection() * RayTCurrent() + getRaySpawnOffsetUsingNormal(normalWorld, nextSampleDirBRDF);
    }
}

