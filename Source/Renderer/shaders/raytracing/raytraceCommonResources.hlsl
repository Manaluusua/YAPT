#ifndef RAYTRACE_COMMON_RESOURCES_HLSL_INCL
#define RAYTRACE_COMMON_RESOURCES_HLSL_INCL

#include "raytraceCommon.hlsl"
#include "../common/random.hlsl"
//data structs, keep in sync with raytracestage

struct RaytraceConstantData
{
	float4x4 uvToView;
	float4x4 viewToWorld;
    float4 worldBoundsMin;
    float4 worldBoundsMax;
	float4 cameraPosition;
	float4 targetTexDimensions;
	float2 rayUVOffset;
	uint maxRayDepth;
    uint envTextureIndex;
    uint envType;
    uint lightCount;
};

struct SpectralDataConstants
{
	float4 spdSampleLambda[(SPECTRAL_SAMPLES_COUNT * SPECTRAL_SAMPLESET_COUNT + 3) / 4];
	float4 spdSamplePdf[(SPECTRAL_SAMPLES_COUNT * SPECTRAL_SAMPLESET_COUNT + 3) / 4];
	uint sampleSetOffset;
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
	uint indexCount;
	uint pad0;
};

struct LightEntryGPU
{
    float4x4 transform;
    float4x4 transformInvTransp;
    float4 centerRadius;
    uint meshIndex;
    uint matIndex;
    uint instanceIndex;
    uint pad0;
};

struct RenderObjectTransformDataGPU
{
    float4 objToWorldR0;
    float4 objToWorldR1;
    float4 objToWorldR2;
    float4 worldToObjectR0;
    float4 worldToObjectR1;
    float4 worldToObjectR2;
	
    float3x4 getObjToWorld()
    {
        return float3x4(objToWorldR0, objToWorldR1, objToWorldR2);
    }
	
    float3x4 getWorldToObj()
    {
        return float3x4(worldToObjectR0, worldToObjectR1, worldToObjectR2);
    }
};




//uniforms
ConstantBuffer<RaytraceConstantData> g_rayGenConstants : register(b1, space0);
ByteAddressBuffer g_randomSampleLocations : register(t2, space0);
ConstantBuffer<SpectralDataConstants> g_spectralSamplingConstants : register(b3, space0);
StructuredBuffer<MaterialEntryGPU> g_materialEntries : register(t4, space0);
StructuredBuffer<MeshEntryGPU> g_meshEntries : register(t5, space0);
StructuredBuffer<RenderObjectTransformDataGPU> g_renderObjectTransforms : register(t6, space0);
ByteAddressBuffer g_renderObjectMatAndMeshIndices : register(t7, space0);
StructuredBuffer<LightEntryGPU> g_lights : register(t8, space0);

SamplerState g_colorSampler : register(s9, space0);
SamplerState g_pointSampler: register(s10, space0);
SamplerState g_lutSampler : register(s11, space0);
Texture2D g_NoiseTex : register(t12, space0);


Texture2D g_dirAlbedoGGXNoFresnelLUT : register(t13, space0);
Texture1D g_avgDirAlbedoGGXNoFresnelLUT : register(t14, space0);
Texture3D g_dirAlbedoGGXSingleAndMultiScatterLUT : register(t15, space0);
Texture2D g_avgDirAlbedoGGXSingleAndMultiScatterLUT : register(t16, space0);
Texture3D g_dirAlbedoGGXTranslucentDenserLUT : register(t17, space0);	
Texture3D g_dirAlbedoGGXTranslucentLighterLUT : register(t18, space0);
Texture2D g_avgAlbedoGGXTranslucentDenserLUT : register(t19, space0);
Texture2D g_avgAlbedoGGXTranslucentLighterLUT : register(t20, space0);
Texture2D g_dirAlbedoSheenNoFresnelLUT : register(t21, space0);

Texture1D g_cieXYZCoeffsLUT : register(t22, space0);
Texture1D g_d65IlluminantLUT : register(t23, space0);
Texture3D g_rec2020ToSPDLUT : register(t24, space0);
Texture3D g_srgbToSPDLUT : register(t25, space0);

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
#define g_rayDirUvOffset g_rayGenConstants.rayUVOffset.xy
#define g_maxRayDepth g_rayGenConstants.maxRayDepth
#define g_targetTexDimensions g_rayGenConstants.targetTexDimensions
#define g_lightCount g_rayGenConstants.lightCount
#define g_worldBoundsMin g_rayGenConstants.worldBoundsMin.xyz
#define g_worldBoundsMax g_rayGenConstants.worldBoundsMax.xyz
#define g_envType g_rayGenConstants.envType
#define g_envTexIndex g_rayGenConstants.envTextureIndex

#define g_randomSamples g_randomSampleLocations

#define g_sampledWavelengths g_spectralSamplingConstants.spdSampleLambda
#define g_sampledWavelengthPDFs g_spectralSamplingConstants.spdSamplePdf


//helper functions

uint scramble(uint random, uint scrambleSeed, uint dimension)
{
    uint h = hash(scrambleSeed, dimension);
    return owenScrambleBase2(random, h);
}

float getRandomSampleFloat(uint dimensionSetIndex, uint seed)
{
    uint startIndex = dimensionSetIndex << 2;
    uint sobolSeq = g_randomSamples.Load(startIndex);
    return uintToFloat01(scramble(sobolSeq, seed, dimensionSetIndex));
}

float2 getRandomSampleFloat2(uint dimensionSetIndex, uint seed)
{
    uint startIndex = dimensionSetIndex << 2;
    uint2 sobolSeq = g_randomSamples.Load2(startIndex);
    sobolSeq.xy = uint2(scramble(sobolSeq.x, seed, dimensionSetIndex), scramble(sobolSeq.y, seed, dimensionSetIndex + 1));
    return uintToFloat01(sobolSeq);
}

float3 getRandomSampleFloat3(uint dimensionSetIndex, uint seed)
{
    uint startIndex = dimensionSetIndex << 2;
    uint3 sobolSeq = g_randomSamples.Load3(startIndex);
    sobolSeq.xyz = uint3(scramble(sobolSeq.x, seed, dimensionSetIndex), scramble(sobolSeq.y, seed, dimensionSetIndex + 1), scramble(sobolSeq.z, seed, dimensionSetIndex + 2));
    return uintToFloat01(sobolSeq);
}

float4 getRandomSampleFloat4(uint dimensionSetIndex, uint seed)
{
    uint startIndex = dimensionSetIndex << 2;
    uint4 sobolSeq = g_randomSamples.Load4(startIndex);
    sobolSeq.xyzw = uint4(scramble(sobolSeq.x, seed, dimensionSetIndex), scramble(sobolSeq.y, seed, dimensionSetIndex + 1), scramble(sobolSeq.z, seed, dimensionSetIndex + 2), scramble(sobolSeq.w, seed, dimensionSetIndex + 3));
    return uintToFloat01(sobolSeq);

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
	uint spectralSampleSetIndex = g_spectralSamplingConstants.sampleSetOffset;
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

uint2 getMaterialAndMeshIndicesForInstance(uint instanceIndex)
{
    uint readOffset = instanceIndex << 3;
    uint2 matMeshIndices = g_renderObjectMatAndMeshIndices.Load2(readOffset);
    return matMeshIndices;
}

RenderObjectTransformDataGPU getTransformDataForInstance(uint instanceIndex)
{
    return g_renderObjectTransforms[instanceIndex];

}

float4 getSkyBoxColor(float3 rayDir, uint envType, uint texIndex)
{
	//keep in sync with c++
	const uint ENVIRONMENT_TYPE_NONE = 0;
	const uint ENVIRONMENT_TYPE_CUBE = 1;
	const uint ENVIRONMENT_TYPE_LONGLAT = 2;
	
	float4 color;

	if (envType == ENVIRONMENT_TYPE_LONGLAT)
	{
		float2 uv = 0; //TODO: calculate from raydir
		color = g_textures2D[NonUniformResourceIndex(texIndex)].SampleLevel(g_colorSampler, uv, 0);
	}
	else if (envType == ENVIRONMENT_TYPE_CUBE)
	{
		color = g_texturesCube[NonUniformResourceIndex(texIndex)].SampleLevel(g_colorSampler, rayDir, 0);
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

float4 getWorldCenterAndRadiusSqr()
{
    float3 c = (g_worldBoundsMax + g_worldBoundsMin) * 0.5f;
    float3 ext = (g_worldBoundsMax - c);
    return float4(c, dot(ext, ext));
}

struct RandomSampler
{
    uint2 dimensionOffsetAndSeed;
	
	void init(uint dim, uint pixelIndex)
	{
        dimensionOffsetAndSeed.x = dim;
        dimensionOffsetAndSeed.y = 0;
    }
		
    float getRandom1()
    {
        float v = getRandomSampleFloat(dimensionOffsetAndSeed.x, dimensionOffsetAndSeed.y);
        ++dimensionOffsetAndSeed.x;
        return v;
    }
	
    float2 getRandom2()
    {
        float2 v = getRandomSampleFloat2(dimensionOffsetAndSeed.x, dimensionOffsetAndSeed.y);
        dimensionOffsetAndSeed.x += 2;
        return v;
    }
    float3 getRandom3()
    {
        float3 v = getRandomSampleFloat3(dimensionOffsetAndSeed.x, dimensionOffsetAndSeed.y);
        dimensionOffsetAndSeed.x += 3;
		return v;
    }
	
    float4 getRandom4()
    {

        float4 v = getRandomSampleFloat4(dimensionOffsetAndSeed.x, dimensionOffsetAndSeed.y);
        dimensionOffsetAndSeed.x += 4;
		return v;
    }
	
    
};


#include "spectralDistribution.hlsl"
#include "rayState.hlsl"


#endif
