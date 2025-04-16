
#include "materialSample.hlsl"
#include "materialModifiers.hlsl"

[shader("closesthit")]
void rayHitDefault(inout Payload payload, in BuiltInTriangleIntersectionAttributes attr)
{
	//Initial surface setup
    float3 barycentrics = float3(1 - attr.barycentrics.x - attr.barycentrics.y, attr.barycentrics.x, attr.barycentrics.y);
	uint3 indices = fetchIndices();
	float3 geometryNormal = fetchMeshNormal(indices, barycentrics);
	float3 tangent = meshHasValidTangents() ? fetchMeshTangent(indices, barycentrics) : float3(1.f, 0.f, 0.f);
	float2 uv = meshHasValidUVs() ? fetchMeshUV(indices, barycentrics) : float2(0.5f, 0.5f);
	
	
	//fetch surface material parameters
	SurfaceDefinitionRGB surfaceDefRGB;
	fetchSurfaceMaterialParameters(surfaceDefRGB);
	
	
	float3 rayDir = ObjectRayDirection();
	rayDir = normalize(rayDir); //ObjectRayDirection() contains scaling (if present)

	float3 normal = geometryNormal;
	modifySurfaceMaterialParametersWithTextures(uv, normal, tangent, surfaceDefRGB);
	SurfaceDefinition surfaceDef = convertSurfaceDefinitionFromRGB(surfaceDefRGB);

	//if two sided, flip normal if view ray hitting from backside
	//when ray too orthogonal to a normal, nudge the normal (if transparent, nudge a bit more since (rough) transparency can generate very high peaks of energy from these cases) 
	{
		float rayDotN = dot(rayDir, geometryNormal);

		if (isTwoSided(surfaceDef.flags))
		{
			if (rayDotN > 0)
			{
				geometryNormal = -geometryNormal;
			}
		}

		float rayOrthogonalThreshold = 0.05f;
		float rayOrthogonalNudgeFactor = 0.05f;

		if (surfaceDef.transparency != 0.f)
		{
			float RAY_ORTHOGONAL_THRESHOLD_MAX_ROUGHNESS = 0.2f;
			float RAY_ORTHOGONAL_NUDGE_FACTOR_MAX_ROUGHNESS = 0.2f;

			rayOrthogonalThreshold = lerp(rayOrthogonalThreshold, RAY_ORTHOGONAL_THRESHOLD_MAX_ROUGHNESS, surfaceDef.roughness);
			rayOrthogonalNudgeFactor = lerp(rayOrthogonalNudgeFactor, RAY_ORTHOGONAL_NUDGE_FACTOR_MAX_ROUGHNESS, surfaceDef.roughness);
		}

		if (abs(rayDotN) < rayOrthogonalThreshold)
		{
			geometryNormal = normalize(geometryNormal - rayDir * rayOrthogonalNudgeFactor);
		}


	}

	
	float3 normalBaseLayer = geometryNormal;
	float3 normalCoatingLayer = geometryNormal;
		
	
	//setup layer transforms
	float3x3 tangentSpaceCoating;
	float3x3 tangentSpaceBaseLayer;
	{
		float3 tangentCoating = tangent;
		makeOrthogonal(normalCoatingLayer, tangentCoating);
		float3 bitangent = cross(tangentCoating, normalCoatingLayer);
		tangentSpaceCoating = float3x3(tangentCoating, normalCoatingLayer, bitangent);
	}
	{
		float3 tangentBase = tangent;
		makeOrthogonal(normalBaseLayer, tangentBase);
		float3 bitangent = cross(tangentBase, normalBaseLayer);
		tangentSpaceBaseLayer = float3x3(tangentBase, normalBaseLayer, bitangent);
	}

	
	
	//add tangent space rotation (could later on optimize out the sin & cos by providing these precalculated on cpu)
	if(surfaceDef.anisotropyRotation > 0)
	{
		float rotAngle = surfaceDef.anisotropyRotation * 2 * PI;
		float cosA = cos(rotAngle);
		float sinA = sin(rotAngle);
		float3x3 rot = float3x3(cosA, 0, sinA,
								0,    1,    0,
								-sinA, 0, cosA );
		tangentSpaceCoating = mul(rot, tangentSpaceCoating);
		tangentSpaceBaseLayer = mul(rot, tangentSpaceBaseLayer);
		
	}
	
	surfaceDef.toCoatingLayerTangentSpace = tangentSpaceCoating;
	surfaceDef.toBaseLayerTangentSpace = tangentSpaceBaseLayer;
	surfaceDef.geometryNormal = geometryNormal;
	
	
	//surface setup done, do the rest
	float3 lightDir;
	SpectralSamples w;

 
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
		
		sampleMaterial(surfaceDef, payload, rayDir,  w, lightDir);
	}
	
	payload.throughput = payload.throughput * w;

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

