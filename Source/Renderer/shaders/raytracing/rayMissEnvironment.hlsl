#include "BPTShared.hlsl"
#include "hitShadersCommon.hlsl"
#include "payload.hlsl"

SpectralSamples ToSpectralSamples(float3 color)
{
	SpectralSamples s;
	s.setFromRGBUnbounded(color);
	return s;
}

[shader("miss")]
void rayMissEnvironment(inout Payload payload)
{
	
    bool outputShadingParams = firstBounceMaterialWriteEnabled();
    if (outputShadingParams && payload.pathLength == 0)
    {
        writeEmptyMaterialParamsForFirstBounce(dispatchIndicesToRayIndices(DispatchRaysIndex().xy), g_MaterialParamsOutput0, g_MaterialParamsOutput1);
    }
	
    g_spectralMainSampleWavelength = payload.sampledWavelength;
	
    float4 color = getSkyBoxColor(WorldRayDirection(), g_envType, g_envTexIndex);
#ifdef WHITE_FURNACE_TEST
	payload.totalLight = payload.totalLight + payload.throughput * ToSpectralSamples(float3(1,1,1));
#else
	payload.totalLight = payload.totalLight + payload.throughput * ToSpectralSamples(color.xyz);
#endif
	//payload.totalLight += payload.throughput * color.a * (rayDir * 0.5 + 0.5) * 0.2 + 0.9;
	payload.rayState = RAY_STATE_TERMINATED; //terminate ray
}
