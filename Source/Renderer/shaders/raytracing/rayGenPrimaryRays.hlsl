
#include "raytraceCommonResources.hlsl"
#include "../utils/common.hlsl"
#include "../utils/random.hlsl"
#include "../utils/commonMath.hlsl"

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


[shader("raygeneration")]
void rayGenPrimaryRays()
{

	
	float2 uv = (float2)DispatchRaysIndex() / ((float2)DispatchRaysDimensions());
    float3 rayDir = generateRayDirection(uv + g_rayDirUvOffset);
	float3 rayOrigin = g_cameraPosition;
	
    Payload payload;
	payload.throughput.set(1.f);
	payload.totalLight.set(0.f);
	payload.rayOrigin = rayOrigin;
    payload.rayDirection = rayDir;
	payload.rayState = RAY_STATE_ALIVE;
	payload.pathLength = 0;
	payload.rayIndex = DispatchRaysIndex().y * DispatchRaysDimensions().x + DispatchRaysIndex().x;
	payload.numberVolumesEntered = 0;
	payload.absorption.set(0.0f);
	
	uint rayFlags = RAY_FLAG_NONE;//RAY_FLAG_CULL_FRONT_FACING_TRIANGLES; //RAY_FLAG_NONE; //RAY_FLAG_CULL_BACK_FACING_TRIANGLES
	uint InstanceInclusionMask = ~0;
	uint RayContributionToHitGroupIndex = 0;
	uint MultiplierForGeometryContributionToHitGroupIndex = 0;
	uint MissShaderIndex = 0;
	
	
	for(uint i = 0; i < g_maxRayDepth; ++i)
	{
		if(payload.rayState == RAY_STATE_ALIVE)
		{
		
			RayDesc ray;
			ray.Origin = payload.rayOrigin;
			ray.Direction = payload.rayDirection;
			ray.TMin = 0.0001f;
			ray.TMax = 1000.0;
		
			TraceRay(g_scene,
			rayFlags,
			InstanceInclusionMask,
			RayContributionToHitGroupIndex,
			MultiplierForGeometryContributionToHitGroupIndex,
			MissShaderIndex,									
			ray,					
			payload);
			
			payload.pathLength += 1;
		}
		else
		{
			break;
		}
		
#ifdef WHITE_FURNACE_TEST_BOUNCE_LIMIT
		if(payload.pathLength >= WHITE_FURNACE_TEST_BOUNCE_LIMIT && payload.rayState != RAY_STATE_TERMINATED)
		{
			SpectralSamples s;
			s.setFromRGBUnbounded(float3(1.f, 1.f, 1.f));

			payload.totalLight = payload.throughput * s;
			payload.rayState = RAY_STATE_TERMINATED;
			break;
		}
		
#endif
		
		//russian roulette
		/*float p = max(payload.throughput.x, max(payload.throughput.y, payload.throughput.z));
		p = max(0.05, 1.f - p);
		float randomSample = vanDerCorputSequence(g_currentRandomSampleIndex + payload.pathLength);
        if (randomSample < p)
		{
			payload.rayState = RAY_STATE_TERMINATED;
            break; 
        }else
		{
			payload.throughput /= max(0.00001f, 1.f - p);
		}*/
       
	}
	
	/*
	if(payload.pathLength == 1)
	{
		g_outputColor[DispatchRaysIndex().xy] = float4(1.0f, 0.0f, 0.0f,  1.f);
	} else if(payload.pathLength < 3)
	{
		g_outputColor[DispatchRaysIndex().xy] = float4(0.0f, 1.0f, 0.0f, 1.f);
	} else if(payload.pathLength < 6)
	{
		g_outputColor[DispatchRaysIndex().xy] = float4(0.0f, 0.0f, 1.0f,  10.f);
	} else if(payload.pathLength < 15)
	{
		g_outputColor[DispatchRaysIndex().xy] = float4(1.0f, 0.0f, 1.0f,  20.f);
	}
	else 
	{
		g_outputColor[DispatchRaysIndex().xy] = float4(1.0f, 1.0f, 1.0f,  1.f);
	}
	return;
	*/
	
	
	/*
	if(payload.numberVolumesEntered == 0 || payload.rayState == RAY_STATE_CANCELLED)
	{
		g_outputColor[DispatchRaysIndex().xy] = float4(1.0f, 0.0f, 0.0f,  1.f);
	} else if(payload.numberVolumesEntered < 2)
	{
		g_outputColor[DispatchRaysIndex().xy] = float4(0.0f, 1.0f, 0.0f, 1.f);
	} else
	{
		g_outputColor[DispatchRaysIndex().xy] = float4(0.0f, 0.0f, 1.0f,  1.f);
	}
	return;
	*/
	
	bool resultsValid = !payload.totalLight.hasNan();
	//if the ray was terminated, write out results. if it was cancelled, don't add samples this frame
	if((payload.rayState == RAY_STATE_TERMINATED) && resultsValid) 
	{
		g_outputColor[DispatchRaysIndex().xy] = float4(payload.totalLight.ToRGB(), 1.f);
	} 
	else
	{
		g_outputColor[DispatchRaysIndex().xy] = float4(0.0f, 0.0f, 0.0f, 0.0f);
		//g_outputColor[DispatchRaysIndex().xy] = float4(10.0f, 0.0f, 0.0f, 10.0f); //flag killed samples visually
		//g_outputColor[DispatchRaysIndex().xy] = float4(payload.totalLight, 10.0f); //flag killed samples visually
	}
	
}