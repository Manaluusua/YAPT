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

struct SpectralDataConstants
{
	float4 spdSampleLambda[(SPECTRAL_SAMPLES_COUNT * SPECTRAL_SAMPLESET_COUNT + 3) / 4];
	float4 spdSamplePdf[(SPECTRAL_SAMPLES_COUNT * SPECTRAL_SAMPLESET_COUNT + 3) / 4];
	uint32_t sampleSetOffset;
};

struct MaterialEntryGPU
{
	float4 specAmountClearCoatAmountIORRoughness;
	float4 albedoTransparency;
	float4 specularMetalness;
	float4 absorptionDielectricIOR;
	float4 emissiveRoughness;

	float anisotropy;
	float anisotropyRotation;
	uint materialMask;
	float thinFilmThickness;

	float2 cauchysCoefficients;
	float sheenAmount;
	float pad0;


	float4 sheenColorRoughness;
	uint2 albedoTexIndexAndScale;
	uint2 normalTexIndexAndScale;
	uint2 ormTexIndexAndScale;
	uint2 emissiveTexIndexAndScale;
};

struct MeshEntryGPU
{
	uint2 positionBuffer;
	uint2 indexBuffer;
	uint2 normalBuffer;
	uint2 tangentBuffer;
	uint2 uvBuffer;
	uint2 pad0;
};

//uniforms
ConstantBuffer<RaytraceConstantData> g_rayGenConstants : register(b1, space0);
ConstantBuffer<RandomSamples> g_randomSampleLocations : register(b2, space0);
ConstantBuffer<SpectralDataConstants> g_spectralSamplingConstants : register(b3, space0);
StructuredBuffer<MaterialEntryGPU> g_materialEntries : register(t4, space0);
StructuredBuffer<MeshEntryGPU> g_meshEntries : register(t5, space0);
RaytracingAccelerationStructure g_accelerationStructure : register(t6, space0);
RWTexture2D<float4> g_outputColor : register(u7, space0);

SamplerState g_colorSampler : register(s8, space0);
SamplerState g_pointSampler: register(s9, space0);
SamplerState g_lutSampler : register(s10, space0);
Texture2D g_NoiseTex : register(t11, space0);


Texture2D g_dirAlbedoGGXNoFresnelLUT : register(t12, space0);
Texture1D g_avgDirAlbedoGGXNoFresnelLUT : register(t13, space0);
Texture3D g_dirAlbedoGGXSingleAndMultiScatterLUT : register(t14, space0);
Texture2D g_avgDirAlbedoGGXSingleAndMultiScatterLUT : register(t15, space0);
Texture3D g_dirAlbedoGGXTranslucentDenserLUT : register(t16, space0);	
Texture3D g_dirAlbedoGGXTranslucentLighterLUT : register(t17, space0);
Texture2D g_avgAlbedoGGXTranslucentDenserLUT : register(t18, space0);
Texture2D g_avgAlbedoGGXTranslucentLighterLUT : register(t19, space0);
Texture2D g_dirAlbedoSheenNoFresnelLUT : register(t20, space0);

Texture1D g_cieXYZCoeffsLUT : register(t21, space0);
Texture1D g_d65IlluminantLUT : register(t22, space0);
Texture3D g_rec2020ToSPDLUT : register(t23, space0);
Texture3D g_srgbToSPDLUT : register(t24, space0);



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

#define g_sampledWavelengths g_spectralSamplingConstants.spdSampleLambda
#define g_sampledWavelengthPDFs g_spectralSamplingConstants.spdSamplePdf

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

float getSpectralSampleLambda(uint index)
{
	uint ind0 = index / 4;
	uint ind1 = index & 0x3;

	return g_sampledWavelengths[ind0][ind1];
}

float getSpectralSampleLambdaPDF(uint index)
{
	uint ind0 = index / 4;
	uint ind1 = index & 0x3;

	return g_sampledWavelengthPDFs[ind0][ind1];
}

uint getSpectralSampleSetIndex()
{
	uint spectralSampleSetIndex = (DispatchRaysIndex().x % 2) + (DispatchRaysIndex().y % 2) * 2;
	spectralSampleSetIndex += g_spectralSamplingConstants.sampleSetOffset;
	spectralSampleSetIndex = spectralSampleSetIndex % SPECTRAL_SAMPLESET_COUNT;
	return spectralSampleSetIndex;
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

MaterialEntryGPU getMaterialEntry(uint index)
{
	return g_materialEntries[index];
}

MeshEntryGPU getMeshEntry(uint index)
{
	return g_meshEntries[index];
}

#include "spectralDistribution.hlsl"
#include "rayState.hlsl"

#endif
