#include "BPTShared.hlsl"
#include "raytraceCommonResources.hlsl"
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
