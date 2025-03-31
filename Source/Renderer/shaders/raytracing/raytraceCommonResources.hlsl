#ifndef RAYTRACE_COMMON_RESOURCES_HLSL_INCL
#define RAYTRACE_COMMON_RESOURCES_HLSL_INCL

#include "raytraceCommon.hlsl"
//data structs, keep in sync with raytracestage

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

struct SpectralSampleWavelengths
{
	float2 lambdaPDF[SPECTRAL_SAMPLES_COUNT];
};

//uniforms
ConstantBuffer<RaytraceConstantData> g_rayGenConstants : register(b1, space0);
ConstantBuffer<RandomSamples> g_randomSampleLocations : register(b2, space0);
ConstantBuffer<SpectralSampleWavelengths> g_sampledWavelengths : register(b3, space0);
RaytracingAccelerationStructure g_accelerationStructure : register(t4, space0);
RWTexture2D<float4> g_outputColor : register(u5, space0);

SamplerState g_colorSampler : register(s6, space0);
SamplerState g_pointSampler: register(s7, space0);
SamplerState g_lutSampler : register(s8, space0);
Texture2D g_NoiseTex : register(t9, space0);


Texture2D g_dirAlbedoGGXNoFresnelLUT : register(t10, space0);
Texture1D g_avgDirAlbedoGGXNoFresnelLUT : register(t11, space0);
Texture3D g_dirAlbedoGGXSingleAndMultiScatterLUT : register(t12, space0);
Texture2D g_avgDirAlbedoGGXSingleAndMultiScatterLUT : register(t13, space0);
Texture3D g_dirAlbedoGGXTranslucentDenserLUT : register(t14, space0);
Texture3D g_dirAlbedoGGXTranslucentLighterLUT : register(t15, space0);
Texture2D g_avgAlbedoGGXTranslucentDenserLUT : register(t16, space0);
Texture2D g_avgAlbedoGGXTranslucentLighterLUT : register(t17, space0);
Texture2D g_dirAlbedoSheenNoFresnelLUT : register(t18, space0);

Texture1D g_cieXYZCoeffsLUT : register(t19, space0);
Texture1D g_rec2020ToSPDLUT : register(t20, space0);
Texture1D g_srgbToSPDLUT : register(t21, space0);



//bindless texture aliases
[[vk::binding(0, 1)]]
Texture2D g_textures2D[] : register(t0, space1);
[[vk::binding(0, 1)]]
TextureCube g_texturesCube[] : register(t0, space10001);


//bindless buffer aliases
[[vk::binding(0, 2)]]
Buffer<uint> g_buffersUint[] : register(t0, space2);
[[vk::binding(0, 2)]]
Buffer<float> g_buffersFloat[] : register(t0, space10002);

//helper defines
#define g_uvToViewTransform g_rayGenConstants.uvToView
#define g_viewToWorldTransform g_rayGenConstants.viewToWorld
#define g_cameraPosition g_rayGenConstants.cameraPosition.xyz
#define g_scene g_accelerationStructure
#define g_rayDirUvOffset g_rayGenConstants.rayUVOffset.xy
#define g_maxRayDepth g_rayGenConstants.maxRayDepth

#define g_currentRandomSampleIndex g_rayGenConstants.currentSampleIndex
#define g_randomSamples g_randomSampleLocations.samples

#define g_sampledWavelengthAndPDF g_sampledWavelengths.lambdaPDF

//helper functions
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

#include "spectralDistribution.hlsl"
#include "payload.hlsl"

#endif
