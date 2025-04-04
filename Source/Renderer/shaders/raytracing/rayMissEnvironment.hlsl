
#include "raytraceCommonResources.hlsl"


struct RayMissShaderTableConstantData
{
	uint envTextureIndex;
	uint envType;
};


SHADERTABLE_EXTRADATA_DECLARE(RayMissShaderTableConstantData);

float4 getSkyBoxColor()
{
	//keep in sync with raytracestage
	const uint ENVIRONMENT_TYPE_NONE = 0;
	const uint ENVIRONMENT_TYPE_CUBE = 1;
	const uint ENVIRONMENT_TYPE_LONGLAT = 2;


	float3 rayDir = WorldRayDirection();
	float4 color;
	uint envType = SHADERTABLE_EXTRADATA.envType;
	if (envType == ENVIRONMENT_TYPE_LONGLAT)
	{
		uint texIndex = SHADERTABLE_EXTRADATA.envTextureIndex;
		float2 uv = 0; //TODO: calculate from raydir
		color = g_textures2D[NonUniformResourceIndex(texIndex)].SampleLevel(g_colorSampler, uv, 0);
	}
	else if (envType == ENVIRONMENT_TYPE_CUBE)
	{
		uint cubemapIndex = SHADERTABLE_EXTRADATA.envTextureIndex;
		color = g_texturesCube[NonUniformResourceIndex(cubemapIndex)].SampleLevel(g_colorSampler, rayDir, 0);
	}
	else
	{
		/*if (abs(dot(rayDir, float3(0, 1, 0))) > 0.707f)
		{
			color = float4(0, 0, 1, 1);
		}
		else
		{
			if (dot(rayDir, float3(1, 0, 0)) > 0)
			{
				color = float4(1, 0, 0, 1);
			}
			else
			{
				color = float4(0, 1, 0, 1);
			}
		}*/
		
		color = float4(abs(rayDir), 1);
		
	}
	return color;
	
}

SpectralSamples ToSpectralSamples(float3 color)
{
	SpectralSamples s;
	s.setFromRGBUnbounded(color);
	return s;
}

[shader("miss")]
void rayMissEnvironment(inout Payload payload)
{
	float4 color = getSkyBoxColor();
#ifdef WHITE_FURNACE_TEST
	payload.totalLight = payload.totalLight + payload.throughput * color.a;
#else
	payload.totalLight = payload.totalLight + payload.throughput * ToSpectralSamples(color.xyz);
#endif
	//payload.totalLight += payload.throughput * color.a * (rayDir * 0.5 + 0.5) * 0.2 + 0.9;
	payload.rayState = RAY_STATE_TERMINATED; //terminate ray
}
