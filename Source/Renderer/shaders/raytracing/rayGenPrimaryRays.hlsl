#include "BPTShared.hlsl"
#include "raytraceCommonResources.hlsl"
#include "../common/common.hlsl"
#include "../common/random.hlsl"
#include "../common/commonMath.hlsl"
#include "payload.hlsl"


[shader("raygeneration")]
void rayGenPrimaryRays()
{
    uint2 rayIndices = dispatchIndicesToRayIndices(DispatchRaysIndex().xy);
	
    Payload payload;
    payload.rayIndex = rayIndices.y * g_targetTexDimensions.x + rayIndices.x;
    RandomSampler rand;
    rand.init(0, payload.rayIndex);

    float2 uv = (float2) rayIndices * g_targetTexDimensions.zw;
    float3 rayDir = generateRayDirection(uv + g_rayDirUvOffset, rand.getTexelOffset());
	float3 rayOrigin = g_cameraPosition;
	
    g_spectralMainSampleWavelength = calculateSpectralSampleWavelength(rand.getRandom1());

	payload.throughput.set(1.f);
	payload.totalLight.set(0.f);
	payload.rayOrigin = rayOrigin;
    payload.rayDirection = rayDir;
	payload.rayState = RAY_STATE_ALIVE;
	payload.pathLength = 0;
	
	payload.numberVolumesEntered = 0;
	payload.flags = 0;
    payload.pdfThisRay = 0;
    payload.randomDimensionOffsetAndScramble = rand.dimensionOffsetAndSeed; //TODO: calculate scrambling here
    payload.sampledWavelength = g_spectralMainSampleWavelength;
	
	#ifdef WHITE_FURNACE_TEST
    payload.rayIndex = -1;
	#endif
	
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
            ray.TMin = DEFAULT_RAY_MIN_T;
            ray.TMax = DEFAULT_RAY_MAX_T;
		
			TraceRay(g_accelerationStructure,
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
		{
            RandomSampler r;
            r.dimensionOffsetAndSeed = payload.randomDimensionOffsetAndScramble;
            if (payload.throughput.applyRussianRoulette(r.getRandom1()))
            {
                payload.rayState = RAY_STATE_TERMINATED;
                break;
            }
            payload.randomDimensionOffsetAndScramble = r.dimensionOffsetAndSeed;
        }
		
       
	}
    g_spectralMainSampleWavelength = payload.sampledWavelength;
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
	if(resultsValid) 
	{
        float3 color = payload.totalLight.ToXYZ();
        g_outputColor[DispatchRaysIndex().xy] = float4(color, 1.f);
    } 
	else
	{
		g_outputColor[DispatchRaysIndex().xy] = float4(0.0f, 0.0f, 0.0f, 0.0f);
		//g_outputColor[DispatchRaysIndex().xy] = float4(10.0f, 0.0f, 0.0f, 10.0f); //flag killed samples visually
		//g_outputColor[DispatchRaysIndex().xy] = float4(payload.totalLight, 10.0f); //flag killed samples visually
	}
	
}