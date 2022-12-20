
#include "materialSample.hlsl"
#include "materialModifiers.hlsl"

[shader("closesthit")]
void rayHitDefault(inout Payload payload, in BuiltInTriangleIntersectionAttributes attr)
{
	//Initial surface setup
    float3 barycentrics = float3(1 - attr.barycentrics.x - attr.barycentrics.y, attr.barycentrics.x, attr.barycentrics.y);
	uint3 indices = fetchIndices();
	float3 geometryNormal = fetchMeshNormal(indices, barycentrics);
	float3 tangent = fetchMeshTangent(indices, barycentrics);
	float2 uv = fetchMeshUV(indices, barycentrics);
	
	
	//fetch surface material parameters
	SurfaceDefinition surfaceDef;
	fetchSurfaceMaterialParameters(surfaceDef);
	
	
	float3 rayDir = ObjectRayDirection();
	rayDir = normalize(rayDir); //ObjectRayDirection() contains scaling (if present)
	
	//if two sided, flip normal if view ray hitting from backside
	if(surfaceDef.isTwoSided)
	{
		if(dot(rayDir, geometryNormal) > 0)
		{
			geometryNormal = -geometryNormal;
		}
	}
	
	float3 normal = geometryNormal;
	
	modifySurfaceMaterialParametersWithTextures(uv, normal, tangent, surfaceDef);
	
	
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
	float3 w;
 
	float rayDistance = RayTCurrent();
	
	//before handling the intersection, apply and clear absorption
	if(dot(payload.absorption, float3(1.f, 1.f, 1.f)) != 0.f)
	{
		payload.throughput *= calculatTransmittance(rayDistance, payload.absorption);
		payload.absorption = float3(0.0f, 0.0f, 0.0f);
		
	}
	
	if(dot(surfaceDef.emissive, float3(1.f, 1.f, 1.f)) != 0.f)
	{
		payload.totalLight += payload.throughput * surfaceDef.emissive;
		payload.rayState = RAY_STATE_TERMINATED;
	}
	else
	{
		
		sampleMaterial(surfaceDef, payload, rayDir,  w, lightDir);
	}
	
	payload.throughput *= w;

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

