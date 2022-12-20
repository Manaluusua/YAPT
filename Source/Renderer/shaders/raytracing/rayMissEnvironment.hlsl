
#include "raytraceCommonResources.hlsl"

struct RayMissShaderTableConstantData
{
	uint environmentMapIndex;
};


SHADERTABLE_EXTRADATA_DECLARE(RayMissShaderTableConstantData);

[shader("miss")]
void rayMissEnvironment(inout Payload payload)
{
	float3 rayDir = WorldRayDirection();
	uint cubemapIndex = SHADERTABLE_EXTRADATA.environmentMapIndex;
	float4 color = g_texturesCube[NonUniformResourceIndex(cubemapIndex)].SampleLevel(g_colorSampler,rayDir,0);
#ifdef WHITE_FURNACE_TEST
	payload.totalLight += payload.throughput * color.a;
#else
	payload.totalLight += payload.throughput * color.xyz;
#endif
	//payload.totalLight += payload.throughput * color.a * (rayDir * 0.5 + 0.5) * 0.2 + 0.9;
	payload.rayState = RAY_STATE_TERMINATED; //terminate ray
}
