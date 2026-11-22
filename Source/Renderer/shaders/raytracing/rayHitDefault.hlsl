#include "BPTShared.hlsl"
#include "hitShadersCommon.hlsl"
#include "materialSample.hlsl"
#include "payload.hlsl"
#include "lightSampling.hlsl"
#include "scatteringSample.hlsl"


struct RayHitShaderTableConstantData
{
	uint2 materialAndMeshIndices;
};
SHADERTABLE_EXTRADATA_DECLARE(RayHitShaderTableConstantData);

struct ExplicitLightSample
{
    SpectralSamples emission; //radiance (with emission profile) for mesh lights, incident intensity for punctual lights
    float3 toLightWS;
    float distance; //DEFAULT_RAY_MAX_T for directional lights
    float pdf; //solid angle pdf including the light pick for mesh lights, only the pick probability for punctual lights
    uint instanceIndex;
    uint primitiveIndex;
    bool isPunctual;
};

//NEE picks from all lights, without NEE only the punctual lights are sampled explicitly (BSDF sampling can never hit them)
bool hasExplicitlySampledLights()
{
#ifdef ENABLE_NEE
    return g_lightCount > 0;
#else
    return g_punctualLightCount > 0;
#endif
}

void sampleExplicitLight(in float3 currentPosWS, in float4 lightSampleRand, out ExplicitLightSample sampleOut)
{
#ifdef ENABLE_NEE
    uint lightIndex = sampleLightIndex(lightSampleRand.w);
    float pickPDF = lightSelectionPDF(lightIndex);
#else
    uint lightIndex = samplePunctualLightIndex(lightSampleRand.w);
    float pickPDF = punctualLightSelectionPDF(lightIndex);
#endif

    LightEntryGPU lightEntry = g_lights[lightIndex];
    if (isPunctualLight(lightEntry))
    {
        PunctualLightIncidence incidence = evaluatePunctualLightIncidence(lightEntry, currentPosWS);
        sampleOut.emission.setFromRGBUnbounded(incidence.intensity);
        sampleOut.toLightWS = incidence.toLightWS;
        sampleOut.distance = incidence.distance;
        sampleOut.pdf = pickPDF;
        sampleOut.instanceIndex = -1;
        sampleOut.primitiveIndex = -1;
        sampleOut.isPunctual = true;
        return;
    }

    LightSampleOutput lightSampleOutput;
    sampleLight(lightIndex, lightSampleRand.xyz, float2(0, 0), lightSampleOutput);

    float3 toLight = lightSampleOutput.positionWS - currentPosWS;
    float toLightLen = length(toLight);

    float cosEmit = -dot(lightSampleOutput.normalWS, toLight / toLightLen);
    sampleOut.emission = lightSampleOutput.radiance * evaluateEmissionProfile(cosEmit, lightSampleOutput.emissionFocus);
    sampleOut.pdf = pickPDF * lightSampleOutput.pdfPos * areaDensityToSolidAngleMultiplier(toLight, lightSampleOutput.normalWS);
    sampleOut.toLightWS = toLight / toLightLen;
    sampleOut.distance = toLightLen;
    sampleOut.instanceIndex = lightSampleOutput.instanceIndex;
    sampleOut.primitiveIndex = lightSampleOutput.primitiveIndex;
    sampleOut.isPunctual = false;

    //if light is not two sided, ignore if we are not facing the light (direction pdf would be wrong but we don't care about it here anyway)
    if(!lightSampleOutput.sampledAsTwoSided && dot(toLight, lightSampleOutput.normalWS) >= 0)
    {
        sampleOut.emission.set(0);
        sampleOut.pdf = 0;
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

    PROCEED_ALPHA_TESTED(q)

    if (q.CommittedStatus() == COMMITTED_TRIANGLE_HIT)
    {

        uint instId = q.CommittedInstanceID();
        uint primIndex = q.CommittedPrimitiveIndex();
        return instId == lightInstanceID && primIndex == lightPrimitiveIndex;
    }
    return false;
}

//punctual lights have no geometry, the light is visible if nothing is hit before reaching it
bool checkUnoccluded(in float3 rayPos, in float3 rayDir, float rayLen)
{
    RayQuery < RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH > q;

    RayDesc ray;
    ray.Origin = rayPos;
    ray.Direction = rayDir;
    ray.TMin = DEFAULT_RAY_MIN_T;
    ray.TMax = rayLen;

    q.TraceRayInline(
		    g_accelerationStructure,
		    RAY_FLAG_NONE,
		    ~0,
		    ray);

    PROCEED_ALPHA_TESTED(q)

    return q.CommittedStatus() != COMMITTED_TRIANGLE_HIT;
}

#ifdef ENABLE_NEE

float calculateExplicitLightConnectionPDF(float3 fromPointToLightUnnormalized, in uint instanceIndex, in uint primitiveIndex, float2 bary)
{
    float lightPickPDF = meshLightSelectionPDFForInstance(instanceIndex);
    
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

    //return pdfSphere();
    
}
#endif
//power heuristic
float weightMIS(float a, float b)
{
    float aSqr = a * a;
    float bSqr = b * b;
    
    return aSqr / (aSqr + bSqr);

}

struct ClosestHitSurface
{
    SurfaceDefinition surfaceDef;
    float3 rayDirObjSpace;
    float2 barycentrics;
    uint materialIndex;
    bool frontFace;
};

struct NextRay
{
    SpectralSamples weight; //already divided by the pdf
    float3 directionWS; //zero if the path ends
    float3 originWS;
};

struct LightConnectionVertex
{
    float3x3 worldToObj;
    float3 woOS;
    float3 positionWS;
    float3 normalWS;
    bool atSubsurfaceExit;
};

//explicit light connection from the vertex. At a subsurface exit point only the exit lobe is evaluated
void connectToExplicitLight(in SurfaceSamplingContext surface, in LightConnectionVertex vertex, inout Payload rayState, inout RandomSampler rand)
{
    float4 randomSamplesLight = rand.getRandom4();
    ExplicitLightSample lightSample;
    sampleExplicitLight(vertex.positionWS, randomSamplesLight, lightSample);

    if(!lightSample.emission.allSamplesEqual(0) && lightSample.pdf > 0)
    {
        float3 toLightWS = lightSample.toLightWS;
        float3 toLightDirOS = normalize(mul(vertex.worldToObj, toLightWS));

        SpectralSamples weightSumLight = (SpectralSamples) 0.f;
        float brdfPdf = 0;
        if (vertex.atSubsurfaceExit)
        {
            evaluateSubsurfaceExitLobe(surface.surfaceDef, surface.precalculatedSurfaceData, vertex.woOS, toLightDirOS, EVALUATE_FLAGS_NONE, weightSumLight, brdfPdf);
        }
        else
        {
            SurfaceEvaluation eval = evaluateSurface(surface, vertex.woOS, toLightDirOS, EVALUATE_FLAGS_NONE);
            weightSumLight = eval.weight;
            brdfPdf = eval.pdf;

            if ((eval.transmissionType & TRANSMISSION_TYPE_DISPERSED) != 0 && !rayState.hasFlags(RAYSTATE_FLAGS_SECONDARY_LAMBDAS_TERMINATED))
            {
                weightSumLight.terminateSecondaryWavelengths();
            }
        }

        if (!weightSumLight.allSamplesEqual(0))
        {
		    float3 rayStart = spawnRayOrigin(vertex.positionWS, vertex.normalWS, toLightWS);
            bool visible;
            float wMIS;
            if (lightSample.isPunctual)
            {
                //delta lights can only be reached by explicit sampling, so there is nothing to weight against
                visible = checkUnoccluded(rayStart, toLightWS, lightSample.distance - DEFAULT_RAY_MIN_T);
                wMIS = 1.f;
            }
            else
            {
                visible = checkLightVisibility(rayStart, toLightWS, lightSample.distance + DEFAULT_RAY_MIN_T, lightSample.instanceIndex, lightSample.primitiveIndex);
                wMIS = weightMIS(lightSample.pdf, brdfPdf);
            }

            if (visible)
            {
                weightSumLight = (weightSumLight / lightSample.pdf) * wMIS;
                rayState.totalLight = rayState.totalLight + rayState.throughput * weightSumLight * lightSample.emission;
            }
        }
    }
}

//continues the path from a subsurface exit point: light connection (NEE) from the exit and the next direction from the exit lobe.
//rayState.throughput must already contain the subsurface entry weight
NextRay continueFromSubsurfaceExit(in SubsurfaceExitSample exit, inout Payload rayState, inout RandomSampler rand)
{
    SurfacePoint exitPoint = exit.exitPoint;
    RenderObjectTransformDataGPU exitTransform = getTransformDataForInstance(exitPoint.instanceIndex);
    float3x3 exitWorldToObj = (float3x3)exitTransform.getWorldToObj();
    float3x3 exitObjToWorld = (float3x3)exitTransform.getObjToWorld();

    //there is no incoming direction at the exit, treat the outward normal as one so that the surface is set up for the outside
    float3 woOS = normalize(mul(exitWorldToObj, exitPoint.geometryNormalWS));

    SurfaceDefinitionRGB exitSurfaceRGB;
    fillSurfaceDefRGB(exitPoint.instanceIndex, exitPoint.primitiveIndex, exitPoint.barycentrics, exitSurfaceRGB);
    handleTwoSidedMaterialOrientation(exitSurfaceRGB, woOS);
    if (rayState.hasFlags(RAYSTATE_FLAGS_REGULARIZE_PATH))
    {
        regularizeMaterial(exitSurfaceRGB);
    }
    SurfaceSamplingContext exitSurface = makeSurfaceSamplingContext(convertSurfaceDefinitionFromRGB(exitSurfaceRGB), rayState.getCurrentIOR(), rayState.getPreviousIOR(), woOS, exitPoint.frontFace);

    if (hasExplicitlySampledLights())
    {
        LightConnectionVertex vertex;
        vertex.worldToObj = exitWorldToObj;
        vertex.woOS = woOS;
        vertex.positionWS = exitPoint.positionWS;
        vertex.normalWS = exitPoint.geometryNormalWS;
        vertex.atSubsurfaceExit = true;
        connectToExplicitLight(exitSurface, vertex, rayState, rand);
    }

    NextRay next;
    next.weight.set(0);
    next.directionWS = 0;
    next.originWS = exitPoint.positionWS;
    float pdf = 0;

    ScatteringEvent ev = sampleScatteringEvent(exitSurface, exitPoint, woOS, true, rand.getRandom4());
    if (ev.type == SCATTERING_EVENT_SUBSURFACE_EXIT_LOBE)
    {
        SpectralSamples weight;
        evaluateSubsurfaceExitLobe(exitSurface.surfaceDef, exitSurface.precalculatedSurfaceData, woOS, ev.wiOS, EVALUATE_FLAGS_NONE, weight, pdf);
        if (pdf > 0)
        {
            next.weight = weight / pdf;
            next.directionWS = normalize(mul(exitObjToWorld, ev.wiOS));
            next.originWS = spawnRayOrigin(exitPoint.positionWS, exitPoint.geometryNormalWS, next.directionWS);
        }
    }

    rayState.addFlags(RAYSTATE_FLAGS_REGULARIZE_PATH);
    rayState.removeFlags(RAYSTATE_FLAGS_SAMPLED_FROM_DELTA_DISTRIBUTION);
    rayState.pdfThisRay = pdf;
    return next;
}

NextRay evaluateSurfaceAndGenerateNextSampleDirection(in ClosestHitSurface hit, inout Payload rayState, inout RandomSampler rand)
{
    float4 randomSamplesBRDF = rand.getRandom4();

    float3 woObjSpace = -hit.rayDirObjSpace;
    SurfaceSamplingContext surface = makeSurfaceSamplingContext(hit.surfaceDef, rayState.getCurrentIOR(), rayState.getPreviousIOR(), woObjSpace, hit.frontFace);

    float3 currentPosWS = calculateHitPositionWS(InstanceID(), PrimitiveIndex(), hit.barycentrics);
    float3x3 worldToObj = (float3x3)WorldToObject3x4();
    float3 normalWS = normalize(mul(transpose(worldToObj), hit.surfaceDef.geometryNormal));

	//Next Event Estimation (explicit light connections)
    if (hasExplicitlySampledLights() && !isDeltaDistribution(surface.precalculatedSurfaceData.a2, hit.surfaceDef))
    {
        LightConnectionVertex vertex;
        vertex.worldToObj = worldToObj;
        vertex.woOS = woObjSpace;
        vertex.positionWS = currentPosWS;
        vertex.normalWS = normalWS;
        vertex.atSubsurfaceExit = false;
        connectToExplicitLight(surface, vertex, rayState, rand);
    }

    SurfacePoint vertexPoint;
    vertexPoint.positionWS = currentPosWS;
    vertexPoint.geometryNormalWS = orientToSameSide(-WorldRayDirection(), normalWS);
    vertexPoint.instanceIndex = InstanceID();
    vertexPoint.primitiveIndex = PrimitiveIndex();
    vertexPoint.barycentrics = hit.barycentrics;
    vertexPoint.frontFace = hit.frontFace;

    ScatteringEvent ev = sampleScatteringEvent(surface, vertexPoint, woObjSpace, false, randomSamplesBRDF);

    if (ev.type == SCATTERING_EVENT_SUBSURFACE_ENTRY)
    {
        //the path continues from the exit point, so its light connection and next direction are handled there, with the entry weight already in the throughput
        ScatteringEvaluation entry = evaluateSubsurfaceEntry(surface, woObjSpace, ev.exit);
        rayState.throughput = rayState.throughput * entry.weight * safeDiv(1.f, entry.pdfForward);
        return continueFromSubsurfaceExit(ev.exit, rayState, rand);
    }

	//evaluate next sample direction (BRDF)
    SurfaceEvaluation eval;
    eval.weight.set(0);
    eval.pdf = 0.f;
    eval.transmissionType = TRANSMISSION_TYPE_NONE;
    bool isDeltaDistr = ev.isDeltaDistribution;
    float3 wiObjSpace = ev.wiOS;
    if (ev.type == SCATTERING_EVENT_LOCAL)
    {
        eval = evaluateSurface(surface, woObjSpace, wiObjSpace, isDeltaDistr ? EVALUATE_FLAGS_TREAT_AS_DELTA : EVALUATE_FLAGS_NONE);

        if (ev.isDiffuseBounce)
        {
            rayState.addFlags(RAYSTATE_FLAGS_REGULARIZE_PATH);
        }
    }

    rayState.pdfThisRay = eval.pdf;

    if (isDeltaDistr)
    {
        rayState.addFlags(RAYSTATE_FLAGS_SAMPLED_FROM_DELTA_DISTRIBUTION);

    }
    else
    {
        rayState.removeFlags(RAYSTATE_FLAGS_SAMPLED_FROM_DELTA_DISTRIBUTION);
    }

    if (eval.pdf > 0.f)
    {

		if (eval.transmissionType != TRANSMISSION_TYPE_NONE)
		{
		    if ((eval.transmissionType & TRANSMISSION_TYPE_DISPERSED) != 0)
		    {

				if (!rayState.hasFlags(RAYSTATE_FLAGS_SECONDARY_LAMBDAS_TERMINATED))
				{
					eval.weight.terminateSecondaryWavelengths();
				}
                rayState.addFlags(RAYSTATE_FLAGS_SECONDARY_LAMBDAS_TERMINATED);
            }

		    bool entered = (eval.transmissionType & TRANSMISSION_TYPE_ENTERED) != 0;
		    bool exited = (eval.transmissionType & TRANSMISSION_TYPE_EXITED) != 0;

		    if (entered != exited)
		    {
		        if (exited)
		        {
		            rayState.exitedVolume(hit.materialIndex);

		        }
		        else if (entered)
		        {
		            rayState.enteredVolume(hit.surfaceDef.dielectricIOR, hit.surfaceDef.absorption, hit.materialIndex);
		        }
		    }


		}

        eval.weight = eval.weight / eval.pdf;
    }

    NextRay next;
    next.weight = eval.weight;
    next.directionWS = 0;
    next.originWS = currentPosWS;
    if (!isZero(wiObjSpace))
    {
        float3x3 objToWorldLin = (float3x3)ObjectToWorld3x4();
        float3 normalWorld = normalize(mul(objToWorldLin, hit.surfaceDef.geometryNormal));
        next.directionWS = normalize(mul(objToWorldLin, wiObjSpace));
        next.originWS = spawnRayOrigin(currentPosWS, normalWorld, next.directionWS);
    }
    return next;
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
    fetchSurfaceMaterialParameters(matEntry, surfaceDefRGB, geometryNormal, normal, normal, tangent);
    handleTwoSidedMaterialOrientation(surfaceDefRGB, -rayDir);
    setupSurfaceOrientation(surfaceDefRGB);
	modifySurfaceMaterialParametersWithTextures(matEntry, uv, normal, tangent, surfaceDefRGB);
    
	// A path with just delta vertices never regularize, which leads to fireflies. Add an option to force regularization after x bounces
    if (shouldStartRegularizing(payload.getPathLength(), g_regularizeAfterVertices))
    {
        payload.addFlags(RAYSTATE_FLAGS_REGULARIZE_PATH);
    }

    if ((payload.getStateFlags() & RAYSTATE_FLAGS_REGULARIZE_PATH) != 0)
    {
        regularizeMaterial(surfaceDefRGB);
    }
    
    

    
    if (payload.pathLength == 0)
    {
        float3x3 transformNormal = (float3x3)WorldToObject3x4();
        transformNormal = transpose(transformNormal);
        float3 hitPosWS = WorldRayOrigin() + WorldRayDirection() * RayTCurrent();
        writeMaterialParamsForFirstBounce(DispatchRaysIndex().xy, surfaceDefRGB, transformNormal, calculateViewDepth(hitPosWS), g_MaterialParamsOutput0, g_MaterialParamsOutput1);
    }

	SurfaceDefinition surfaceDef = convertSurfaceDefinitionFromRGB(surfaceDefRGB);

	//surface setup done, do the rest
	float rayDistance = RayTCurrent();
    
    handleRayStartInsideMedium(payload, surfaceDef, materialAndMeshIndices.x, rayDir, geometryNormal);
    
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
        if (g_lightCount > 0 && payload.pdfThisRay > 0 && !payload.hasFlags(RAYSTATE_FLAGS_SAMPLED_FROM_DELTA_DISTRIBUTION))
        {
            float lightPDF = calculateExplicitLightConnectionPDF(WorldRayOrigin() + WorldRayDirection() * RayTCurrent() - payload.rayOrigin, InstanceID(), PrimitiveIndex(), attr.barycentrics);
            wMIS = weightMIS(payload.pdfThisRay, lightPDF);
        }
		
#endif
        float sideMultiplier = dot(surfaceDef.geometryNormal, -rayDir) < 0 ? 0 : 1;
        float emissionProfile = 1.f;
        if (surfaceDef.emissionFocus > 0.f)
        {
			//object space normal/direction cosines are not preserved under non uniform scaling, so do this in world space
            float3 geometryNormalWS = normalize(mul(surfaceDef.geometryNormal, (float3x3) WorldToObject3x4()));
            emissionProfile = evaluateEmissionProfile(dot(geometryNormalWS, -normalize(WorldRayDirection())), surfaceDef.emissionFocus);
        }
        payload.totalLight = payload.totalLight + payload.throughput * surfaceDef.emissive * wMIS * sideMultiplier * emissionProfile;
		payload.rayState = RAY_STATE_TERMINATED;
	}
	else
	{
#ifdef WHITE_FURNACE_TEST
        if (!handleWhiteFurnaceTest(payload))
        {
            return;
        }
#endif
        
        
        RandomSampler rand;
        rand.dimensionOffsetAndSeed = payload.randomDimensionOffsetAndScramble;
        
        ClosestHitSurface hit;
        hit.surfaceDef = surfaceDef;
        hit.rayDirObjSpace = rayDir;
        hit.barycentrics = attr.barycentrics;
        hit.materialIndex = materialAndMeshIndices.x;
        hit.frontFace = HitKind() == HIT_KIND_TRIANGLE_FRONT_FACE;
        NextRay next = evaluateSurfaceAndGenerateNextSampleDirection(hit, payload, rand);
        payload.throughput = payload.throughput * next.weight;

        payload.randomDimensionOffsetAndScramble = rand.dimensionOffsetAndSeed;

        if (isZero(next.directionWS) || payload.pdfThisRay == 0.0f)
        {
            payload.rayState = RAY_STATE_CANCELLED;
        }
        else
        {
            payload.rayDirection = next.directionWS;
            payload.rayOrigin = next.originWS;
        }
    }
}

