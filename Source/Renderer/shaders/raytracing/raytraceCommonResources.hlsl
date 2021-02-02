#ifndef RAYTRACE_COMMON_RESOURCES_HLSL_INCL
#define RAYTRACE_COMMON_RESOURCES_HLSL_INCL

#include "raytraceCommon.hlsl"

#define NUMBER_OF_RANDOM_SAMPLES 256

#define RAY_STATE_ALIVE 0
#define RAY_STATE_TERMINATED 1
#define RAY_STATE_CANCELLED 2
#define RAY_MAX_VOLUMES_ENTERED 4

#define IOR_DEFAULT (1.0f) 

//structures
struct Payload
{
	float3 throughput;
	uint rayState; 
	float3 totalLight;
	uint pathLength;
	float3 rayOrigin;
	uint rayIndex;
	float3 rayDirection;
	uint numberVolumesEntered;
	float volumesEntered[RAY_MAX_VOLUMES_ENTERED];
	float3 absorption;
};

struct RaytraceConstantData
{
	float4x4 uvToView;
	float4x4 viewToWorld;
	float4 cameraPosition;
	float2 rayUVOffset;
	uint currentSampleIndex;
	uint maxRayDepth;
};

struct RandomSamples
{
	float4 samples[NUMBER_OF_RANDOM_SAMPLES];
};



ConstantBuffer<RaytraceConstantData> g_rayGenConstants : register(b1, space0);
ConstantBuffer<RandomSamples> g_randomSampleLocations : register(b2, space0);
RaytracingAccelerationStructure g_accelerationStructure : register(t3, space0);
RWTexture2D<float4> g_outputColor : register(u4, space0);
SamplerState g_colorSampler : register(s6, space0);
SamplerState g_pointSampler: register(s7, space0);
SamplerState g_lutSampler : register(s8, space0);

Texture2D g_dirAlbedoGGXNoFresnelLUT : register(t10, space0);
Texture1D g_avgDirAlbedoGGXNoFresnelLUT : register(t11, space0);
Texture3D g_dirAlbedoGGXSingleAndMultiScatterLUT : register(t12, space0);
Texture2D g_avgDirAlbedoGGXSingleAndMultiScatterLUT : register(t13, space0);
Texture3D g_dirAlbedoGGXTranslucentDenserLUT : register(t14, space0);
Texture3D g_dirAlbedoGGXTranslucentLighterLUT : register(t15, space0);
Texture2D g_avgAlbedoGGXTranslucentDenserLUT : register(t16, space0);
Texture2D g_avgAlbedoGGXTranslucentLighterLUT : register(t17, space0);
Texture2D g_dirAlbedoSheenNoFresnelLUT : register(t18, space0);

Texture2D g_noiseTex : register(t20, space0);


//bindless texture aliases
Texture2D g_textures2D[] : register(t0, space1);
TextureCube g_texturesCube[] : register(t0, space10001);


//bindless buffer aliases
Buffer<uint> g_buffersUint[] : register(t0, space2);
Buffer<float> g_buffersFloat[] : register(t0, space10002);



//defines
#define g_uvToViewTransform g_rayGenConstants.uvToView
#define g_viewToWorldTransform g_rayGenConstants.viewToWorld
#define g_cameraPosition g_rayGenConstants.cameraPosition.xyz
#define g_scene g_accelerationStructure
#define g_rayDirUvOffset g_rayGenConstants.rayUVOffset.xy
#define g_maxRayDepth g_rayGenConstants.maxRayDepth

#define g_currentRandomSampleIndex g_rayGenConstants.currentSampleIndex
#define g_randomSamples g_randomSampleLocations.samples

//funcs
float4 getRandomSampleFloat4(uint offset)
{
	uint index = offset % NUMBER_OF_RANDOM_SAMPLES;
	return g_randomSamples[index];
}

float2 getRandomSampleFloat2(uint offset)
{
	return getRandomSampleFloat4(offset).xy;
}
float3 getRandomSampleFloat3(uint offset)
{
	return getRandomSampleFloat4(offset).xyz;
}

float4 sampleLUT(in SamplerState s, in Texture1D t, float uv)
{
	float dim;
	t.GetDimensions(dim);
	float dimInv = 1.f/dim;
	
	float scale = (dim - 1.f) * dimInv;
	float bias = dimInv * 0.5f;
	float c = uv * scale + bias;
	
	return t.SampleLevel(s, c, 0);
}

float4 sampleLUT(in SamplerState s, in Texture2D t, float2 uv)
{
	float2 dim;
	t.GetDimensions(dim.x, dim.y);
	float2 dimInv = 1.f/dim;
	
	float2 scale = (dim - 1.f) * dimInv;
	float2 bias = dimInv * 0.5f;
	float2 c = uv * scale + bias;
	
	return t.SampleLevel(s, c, 0);
}

float4 sampleLUT(in SamplerState s, in Texture3D t, float3 uv)
{
	float3 dim;
	t.GetDimensions(dim.x, dim.y, dim.z);
	float3 dimInv = 1.f/dim;
	
	float3 scale = (dim - 1.f) * dimInv;
	float3 bias = dimInv * 0.5f;
	float3 c = uv * scale + bias;
	
	return t.SampleLevel(s, c, 0);
}


float payloadGetCurrentIOR(in Payload payload)
{
	float currentIOR = IOR_DEFAULT; //air if not entered volume
	if(payload.numberVolumesEntered != 0)
	{
		currentIOR = payload.volumesEntered[payload.numberVolumesEntered - 1];
	}
	return currentIOR;
}

float payloadGetBeforeCurrentIOR(in Payload payload)
{
	float beforeCurrentIOR = IOR_DEFAULT; 
	if(payload.numberVolumesEntered > 1)
	{
		beforeCurrentIOR = payload.volumesEntered[payload.numberVolumesEntered - 2];
	}
	return beforeCurrentIOR;
}

void payloadRayEnteredVolume(inout Payload payload, in float IOROfEnteredVolume)
{
	payload.numberVolumesEntered = min(payload.numberVolumesEntered + 1, RAY_MAX_VOLUMES_ENTERED);
	payload.volumesEntered[payload.numberVolumesEntered - 1] = IOROfEnteredVolume;
}

void payloadRayExitedVolume(inout Payload payload)
{
	payload.numberVolumesEntered = max(0, payload.numberVolumesEntered - 1);
}



#endif
