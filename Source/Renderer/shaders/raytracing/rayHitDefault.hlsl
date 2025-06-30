#include "BPTShared.hlsl"
#include "materialSample.hlsl"
#include "payload.hlsl"


struct RayHitShaderTableConstantData
{
	uint2 materialAndMeshIndices;
};
SHADERTABLE_EXTRADATA_DECLARE(RayHitShaderTableConstantData);

#ifdef ENABLE_NEE
void sampleExplicitLightDir(in float3 currentPosWS, in uint lightIndex, in float2 randLightDir, out uint lightInstanceIdOut, out float3 toLightDirWSOut, out float lightRayMaxT, out float pdfOut)
{
    
    LightEntryGPU lightEntry = g_lights[lightIndex];
    float4 sphere = lightEntry.centerRadius;
	
    float3 toLight = normalize(sphere.xyz - currentPosWS);
	
    float3 v1, v2;
    constructVectorBase(toLight, v1, v2);
    float2 cd = sampleConcentricDisk(randLightDir.xy);
    float3 lightSamplePos = sphere.xyz + sphere.w * (cd.x * v1 + cd.y * v2);
	
    float3 toLightRay = lightSamplePos - currentPosWS;
    float toLightRayLen = length(toLightRay);
	
    toLightDirWSOut = toLightRay / toLightRayLen;
    lightRayMaxT = toLightRayLen + sphere.w;
    pdfOut = 1 / (PI * sphere.w * sphere.w);

    //from area to solid angle
    pdfOut *= toLightRayLen * toLightRayLen;
    
    lightInstanceIdOut = lightEntry.instanceIndex;

}

bool evaluateLightEmission(in float3 rayPos, in float3 rayDir, float rayLen, uint lightInstanceID, out SpectralSamples emission)
{
    uint rayFlags = RAY_FLAG_NONE;
    uint InstanceInclusionMask = ~0;
    RayQuery < RAY_FLAG_NONE > q;

    RayDesc ray;
    ray.Origin = rayPos;
    ray.Direction = rayDir;
    ray.TMin = 0.001f;
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
        SurfaceDefinition surfaceDef;
        PrecalculatedSurfaceData precalculatedSurfaceData;
        bool triangleHitFrontFace = q.CommittedTriangleFrontFace();
        uint instId = q.CommittedInstanceID();
        uint primIndex = q.CommittedPrimitiveIndex();
        float2 bary = q.CommittedTriangleBarycentrics();
        
        
        if (lightInstanceID != instId)
        {
            emission.set(0);
            return false;
        }

        uint2 matMeshIndices = getMaterialAndMeshIndices(instId);
        MeshEntryGPU meshEntry = getMeshEntry(matMeshIndices.y);

	    //Initial surface setup
        float3 barycentrics = float3(1 - bary.x - bary.y, bary.x, bary.y);
        uint3 indices = fetchIndices(meshEntry.indexBuffer, primIndex);
        float3 geometryNormal = fetchMeshNormal(meshEntry.normalBuffer, indices, barycentrics);
        float3 tangent = meshHasValidTangents(meshEntry.tangentBuffer) ? fetchMeshTangent(meshEntry.tangentBuffer, indices, barycentrics) : float3(1.f, 0.f, 0.f);
        float2 uv = meshHasValidUVs(meshEntry.uvBuffer) ? fetchMeshUV(meshEntry.uvBuffer, indices, barycentrics) : float2(0.5f, 0.5f);

        float3 normal = geometryNormal;

	    //fetch surface material parameters
        MaterialEntryGPU matEntry = getMaterialEntry(matMeshIndices.x);
        SurfaceDefinitionRGB surfaceDefRGB;
        fetchSurfaceMaterialParameters(matEntry, surfaceDefRGB);
        modifySurfaceMaterialParametersWithTextures(matEntry, uv, normal, tangent, surfaceDefRGB);
        surfaceDef = convertSurfaceDefinitionFromRGB(surfaceDefRGB);
        emission = surfaceDef.emissive;
        return true;
    }
    return false;
}

float calculateExplicitLightPDF(in uint lightIndex, float3 wPos, float3 wDir)
{
    LightEntryGPU lightEntry = g_lights[lightIndex];
    float4 sphere = lightEntry.centerRadius;
    
    float3 toLight = sphere.xyz - wPos;
    float toLightLen = length(toLight);
    float3 toLightDir = toLight / toLightLen;
    float t = toLightLen / dot(toLightDir, wDir);
    
    float3 pOnDisk = wPos + t * wDir;
    
    float3 toPointFromCenter = pOnDisk - sphere.xyz;
    float lenSqr = dot(toPointFromCenter, toPointFromCenter);
    
    if (lenSqr > sphere.w * sphere.w)
    {
        return 0;
    } 
    else
    {
        //disk position sampler probability converted to solid angle probability
        return t * t / (PI * sphere.w * sphere.w); 
    }
    
}
#endif
//power heuristic
float weightMIS(float a, float b)
{
    float aSqr = a * a;
    float bSqr = b * b;
    return aSqr / (aSqr + bSqr);

}

uint getRaySampleIndex(in Payload rayState, in int pathLengthOffset = 0)
{
    return g_currentRandomSampleIndex + (DispatchRaysIndex().y * 7) + (rayState.getPathLength() + pathLengthOffset) * 5 + rayState.getRayIndex() * 11;
}

void evaluateSurfaceAndGenerateNextSampleDirection(in SurfaceDefinition surfaceDef, inout Payload rayState, in float3 rayDirObjSpace, in bool triangleHitFrontFace, out SpectralSamples weightOut, out float3 nextSampleDirOut)
{
	
    uint sampleIndex = getRaySampleIndex(rayState);
    float4 randomSamplesBRDF = getRandomSampleFloat4(sampleIndex, 0);
	
    PrecalculatedSurfaceData precalculatedSurfData;
    getPrecalculatedSurfaceData(surfaceDef, rayState.getCurrentIOR(), rayState.getPreviousIOR(), -rayDirObjSpace, triangleHitFrontFace, precalculatedSurfData);

    float samplingProbabilities[LAYER_COUNT];
    calculateNormalizedMaterialLayerSamplingProbabilities(surfaceDef, precalculatedSurfData, randomSamplesBRDF.xy, samplingProbabilities);
    
	//Next Event Estimation (explicit light connections)
#ifdef ENABLE_NEE
    int lightIndex = -1;
    float3 currentPosWS = WorldRayOrigin() + WorldRayDirection() * RayTCurrent();
    
    if (g_lightCount > 0)
    {
        float4 randomSamplesLight = getRandomSampleFloat4(sampleIndex, 1);
        lightIndex = min((uint) floor(randomSamplesLight.w * g_lightCount), g_lightCount - 1);

        float3 explicitLightDirWS;
        float lightRayMaxT;
        uint lightInstanceId;
        float pdfLightDir;
        float pdfBRDF;
        sampleExplicitLightDir(currentPosWS, (uint) lightIndex, randomSamplesLight.xy, lightInstanceId, explicitLightDirWS, lightRayMaxT, pdfLightDir);
    
        pdfLightDir *= 1.f/g_lightCount;

        float3x3 toOSLight = (float3x3)WorldToObject3x4();
        float3 toLightDirOS = mul(toOSLight, explicitLightDirWS);
		
        SpectralSamples weightSumLight = (SpectralSamples) 0.f;
        TransmissionType transmissionTypeDummy;
        evaluateSurface(surfaceDef, -rayDirObjSpace, toLightDirOS, samplingProbabilities, precalculatedSurfData, false, weightSumLight, pdfBRDF, transmissionTypeDummy);
		
		
        if (pdfLightDir > 0 && !weightSumLight.allSamplesEqual(0))
        {
            SpectralSamples emission;
			
            if (evaluateLightEmission(currentPosWS, explicitLightDirWS, lightRayMaxT, lightInstanceId, emission))
            {
                weightSumLight = (weightSumLight / pdfLightDir) * weightMIS(pdfLightDir, pdfBRDF);
                rayState.totalLight = rayState.totalLight + rayState.throughput * weightSumLight * emission;
            }

        }
    }
#endif

	//evaluate next sample direction (BRDF)
    SpectralSamples weightSumBRDF = (SpectralSamples) 0.f;
    float pdfBRDF = 0.f;
    float pdfLightDir = 0.f;
    TransmissionType transmissionType;
    float3 wiObjSpace = getSampleDirectionOS(surfaceDef, precalculatedSurfData, randomSamplesBRDF.w, randomSamplesBRDF.xy, samplingProbabilities);
    if (!isZero(wiObjSpace))
    {
        evaluateSurface(surfaceDef, -rayDirObjSpace, wiObjSpace, samplingProbabilities, precalculatedSurfData, false, weightSumBRDF, pdfBRDF, transmissionType);
    }
	
    if (pdfBRDF > 0.f)
     {
        rayState.pdfThisRay = pdfBRDF;
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


[shader("closesthit")]
void rayHitDefault(inout Payload payload, in BuiltInTriangleIntersectionAttributes attr)
{
	uint2 materialAndMeshIndices = SHADERTABLE_EXTRADATA.materialAndMeshIndices;
	MeshEntryGPU meshEntry = getMeshEntry(materialAndMeshIndices.y);


	//Initial surface setup
    float3 barycentrics = float3(1 - attr.barycentrics.x - attr.barycentrics.y, attr.barycentrics.x, attr.barycentrics.y);
	uint3 indices = fetchIndices(meshEntry.indexBuffer, PrimitiveIndex());
	float3 geometryNormal = fetchMeshNormal(meshEntry.normalBuffer, indices, barycentrics);
	float3 tangent = meshHasValidTangents(meshEntry.tangentBuffer) ? fetchMeshTangent(meshEntry.tangentBuffer, indices, barycentrics) : float3(1.f, 0.f, 0.f);
	float2 uv = meshHasValidUVs(meshEntry.uvBuffer) ? fetchMeshUV(meshEntry.uvBuffer, indices, barycentrics) : float2(0.5f, 0.5f);
	
	
	float3 rayDir = ObjectRayDirection();
	rayDir = normalize(rayDir); //ObjectRayDirection() contains scaling (if present)

	float3 normal = geometryNormal;

	//fetch surface material parameters
	MaterialEntryGPU matEntry = getMaterialEntry(materialAndMeshIndices.x);
	SurfaceDefinitionRGB surfaceDefRGB;
	fetchSurfaceMaterialParameters(matEntry, surfaceDefRGB);

	modifySurfaceMaterialParametersWithTextures(matEntry, uv, normal, tangent, surfaceDefRGB);
	
    normal = nudgeNormal(rayDir, normal, surfaceDefRGB);
    geometryNormal = nudgeNormal(rayDir, geometryNormal, surfaceDefRGB);
	
    setupSurfaceOrientation(geometryNormal, normal, normal, tangent, tangent, surfaceDefRGB);
	SurfaceDefinition surfaceDef = convertSurfaceDefinitionFromRGB(surfaceDefRGB);

	//surface setup done, do the rest
	float3 nextSampleDirBRDF;
	float rayDistance = RayTCurrent();
	
	//before handling the intersection, apply and clear absorption
	if (!payload.absorption.allSamplesEqual(0))
	{

		payload.throughput = payload.throughput * calculateTransmittance(rayDistance, payload.absorption);
		payload.absorption.set(0.f);
		
	}
	
	if(!surfaceDef.emissive.allSamplesEqual(0))
    {
        float wMIS = 1.f;
#ifdef ENABLE_NEE
        if (g_lightCount > 0 && payload.pdfThisRay > 0)
        {
			//evaluate PDF for explicit light		
            float3 wiWS = WorldRayDirection();
            float3 rayOriginWS = WorldRayOrigin(); 
        
            uint sampleIndex = getRaySampleIndex(payload, -1);
            float4 randomSamplesLight = getRandomSampleFloat4(sampleIndex, 1);
            uint lightIndex = min((uint) floor(randomSamplesLight.w * g_lightCount), g_lightCount - 1);
        
            float lightPDF = calculateExplicitLightPDF(lightIndex, rayOriginWS, wiWS);
            lightPDF *= 1.f/g_lightCount;
            wMIS = weightMIS(payload.pdfThisRay, lightPDF);
        }
		
#endif
        
        
		payload.totalLight = payload.totalLight + payload.throughput * surfaceDef.emissive * wMIS;
		payload.rayState = RAY_STATE_TERMINATED;
	}
	else
	{
		
        SpectralSamples w;
        evaluateSurfaceAndGenerateNextSampleDirection(surfaceDef, payload, rayDir, HitKind() == HIT_KIND_TRIANGLE_FRONT_FACE, w, nextSampleDirBRDF);
        payload.throughput = payload.throughput * w;
        
        if (isZero(nextSampleDirBRDF))
        {
            payload.rayState = RAY_STATE_TERMINATED;
        }
    }
	
	

    

	if(payload.rayState == RAY_STATE_ALIVE)
	{
		float3x3 objToWorldLin =  (float3x3)ObjectToWorld3x4();
	
		float offsetEpsilon = 0.001f;
        bool transmitted = dot(nextSampleDirBRDF, geometryNormal) < 0.f ? true : false;
		
		float3 normalWorld = mul(objToWorldLin, geometryNormal);
		normalWorld = normalize(normalWorld);
		
		float3 rayOffset = normalWorld * offsetEpsilon;
		rayOffset *= transmitted ? -1.f : 1.f;
	
        nextSampleDirBRDF = mul(objToWorldLin, nextSampleDirBRDF);
        nextSampleDirBRDF = normalize(nextSampleDirBRDF);

        payload.rayDirection = nextSampleDirBRDF;
		payload.rayOrigin = WorldRayOrigin() + WorldRayDirection() * RayTCurrent() + rayOffset;
	}
}

