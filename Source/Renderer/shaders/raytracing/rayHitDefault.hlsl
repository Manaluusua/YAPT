#include "BPTShared.hlsl"
#include "materialSample.hlsl"
#include "payload.hlsl"

struct RayHitShaderTableConstantData
{
	uint2 materialAndMeshIndices;
};
SHADERTABLE_EXTRADATA_DECLARE(RayHitShaderTableConstantData);

void evaluateSurfaceAndGenerateNextSampleDirection(in SurfaceDefinition surfaceDef, inout Payload rayState, in float3 rayDirObjSpace, in float4 randomSamples, in bool triangleHitFrontFace, out SpectralSamples weightOut, out float3 nextSampleDirOut)
{
    PrecalculatedSurfaceData precalculatedSurfData;
    getPrecalculatedSurfaceData(surfaceDef, rayState.getCurrentIOR(), rayState.getPreviousIOR(), -rayDirObjSpace, triangleHitFrontFace, precalculatedSurfData);

    float samplingProbabilities[LAYER_COUNT];
    calculateNormalizedMaterialLayerSamplingProbabilities(surfaceDef, precalculatedSurfData, randomSamples.xy, samplingProbabilities);
    float3 wiObjSpace = getSampleDirectionOS(surfaceDef, precalculatedSurfData, randomSamples.w, randomSamples.xy, samplingProbabilities);

    SpectralSamples weightSum = (SpectralSamples) 0.f;
    float pdfSum = 0.f;

    if (!isZero(wiObjSpace))
    {
		
        TransmissionType transmissionType;
        evaluateSurface(surfaceDef, -rayDirObjSpace, wiObjSpace, samplingProbabilities, precalculatedSurfData, false, weightSum, pdfSum, transmissionType);

        if (pdfSum > 0.f)
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

            weightSum = weightSum / pdfSum;
        }
    }

    weightOut = weightSum;
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
	float3 lightDir;
	float rayDistance = RayTCurrent();
	
	//before handling the intersection, apply and clear absorption
	if (!payload.absorption.allSamplesEqual(0))
	{

		payload.throughput = payload.throughput * calculateTransmittance(rayDistance, payload.absorption);
		payload.absorption.set(0.f);
		
	}
	
	if(!surfaceDef.emissive.allSamplesEqual(0))
	{
		payload.totalLight = payload.totalLight + payload.throughput * surfaceDef.emissive;
		payload.rayState = RAY_STATE_TERMINATED;
	}
	else
	{
		uint sampleIndex = g_currentRandomSampleIndex + payload.getPathLength() * 7 + payload.getRayIndex() * 11;
		float4 randomSamples = getRandomSampleFloat4(sampleIndex, 0);
        SpectralSamples w;
		
		evaluateSurfaceAndGenerateNextSampleDirection(surfaceDef, payload, rayDir, randomSamples, HitKind() == HIT_KIND_TRIANGLE_FRONT_FACE,  w, lightDir);
        payload.throughput = payload.throughput * w;
    }
	
	

	if(isZero(lightDir))
	{
		payload.rayState = RAY_STATE_TERMINATED;
	}

	if(payload.rayState == RAY_STATE_ALIVE)
	{
		float3x3 objToWorldLin =  (float3x3)ObjectToWorld3x4();
	
		float offsetEpsilon = 0.001f;
		bool transmitted = dot(lightDir, geometryNormal) < 0.f ? true: false;
		
		float3 normalWorld = mul(objToWorldLin, geometryNormal);
		normalWorld = normalize(normalWorld);
		
		float3 rayOffset = normalWorld * offsetEpsilon;
		rayOffset *= transmitted ? -1.f : 1.f;
	
		lightDir = mul(objToWorldLin, lightDir);
		lightDir = normalize(lightDir);

		payload.rayDirection = lightDir;
		payload.rayOrigin = WorldRayOrigin() + WorldRayDirection() * RayTCurrent() + rayOffset;
	}
}

