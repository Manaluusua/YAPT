#ifndef MATERIALS_COMMON_RESOURCES_HLSL_INCL
#define MATERIALS_COMMON_RESOURCES_HLSL_INCL

#include "../globalDefinitions.hlsl"
#include "../common/commonMath.hlsl"
#include "../common/random.hlsl"


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
StructuredBuffer<MaterialEntryGPU> g_materialEntries : register(t6, space0);

StructuredBuffer<MeshEntryGPU> g_meshEntries : register(t7, space0);

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

#endif
