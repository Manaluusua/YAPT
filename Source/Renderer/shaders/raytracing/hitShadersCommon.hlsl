#ifndef HITSHADERSCOMMON_HLSL_INCL
#define HITSHADERSCOMMON_HLSL_INCL

#include "raytraceCommonResources.hlsl"

#define MaterialMask_TwoSided (1 << 0)
#define MaterialMask_Dispersion (1 << 1)
#define TEX_UNBOUND_INDEX (~0)

struct RayHitShaderTableConstantData
{
	//vertex data
	uint2 indexBuffer;
	uint2 normalBuffer;
	uint2 tangentBuffer;
	uint2 uvBuffer;
	
	//material data
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
	uint2 albedoTexIndex;
	uint2 normalTexIndex;
	uint2 ormTexIndex;
	uint2 emissiveTexIndex;
};

struct SurfaceDefinitionRGB
{
	float3x3 toCoatingLayerTangentSpace;
	float3x3 toBaseLayerTangentSpace;

	float3 specular;
	float metalness;
	float3 albedo;
	float transparency;
	float3 absorption;
	float dielectricIOR;
	float3 emissive;
	float roughness;
	float3 geometryNormal;
	float specularAmount;
	float clearCoatAmount;
	float clearCoatIOR;
	float clearCoatRoughness;

	float anisotropy;
	float anisotropyRotation;
	float thinFilmThickness;
	float2 cauchysCoeffs;
	float3 sheenColor;
	float sheenRoughness;
	float sheenAmount;
	uint flags;
};

struct SurfaceDefinition
{
	float3x3 toCoatingLayerTangentSpace;
	float3x3 toBaseLayerTangentSpace;
	
	SpectralSamples specular;
	SpectralSamples albedo;
	SpectralSamples absorption;
	SpectralSamples emissive;
	SpectralSamples sheenColor;
	float3 geometryNormal;
	float transparency;
	
	float metalness;
	float dielectricIOR;
	float roughness;
	float specularAmount;
	float clearCoatAmount;
	float clearCoatIOR;
	float clearCoatRoughness;

	float anisotropy;
	float anisotropyRotation;
	float thinFilmThickness;
	float sheenRoughness;
	float sheenAmount;
	float2 cauchysCoeffs;
	uint flags;
};

SurfaceDefinition convertSurfaceDefinitionFromRGB(SurfaceDefinitionRGB rgb)
{
	SurfaceDefinition surfDef;
	surfDef.toCoatingLayerTangentSpace = rgb.toCoatingLayerTangentSpace;
	surfDef.toBaseLayerTangentSpace = rgb.toBaseLayerTangentSpace;

	surfDef.specular.setFromRGB(rgb.specular);
	surfDef.albedo.setFromRGB(rgb.albedo);
	surfDef.absorption.setFromRGB(rgb.absorption);
	surfDef.emissive.setFromRGBUnbounded(rgb.emissive);
	surfDef.sheenColor.setFromRGB(rgb.sheenColor);

	surfDef.geometryNormal = rgb.geometryNormal;

	surfDef.transparency = rgb.transparency;
	surfDef.metalness = rgb.metalness;
	surfDef.dielectricIOR = rgb.dielectricIOR;
	surfDef.roughness = rgb.roughness;
	surfDef.specularAmount = rgb.specularAmount;

	surfDef.clearCoatAmount = rgb.clearCoatAmount;
	surfDef.clearCoatIOR = rgb.clearCoatIOR;
	surfDef.clearCoatRoughness = rgb.clearCoatRoughness;

	surfDef.anisotropy = rgb.anisotropy;
	surfDef.anisotropyRotation = rgb.anisotropyRotation;
	surfDef.thinFilmThickness = rgb.thinFilmThickness;

	surfDef.cauchysCoeffs = rgb.cauchysCoeffs;

	surfDef.sheenRoughness = rgb.sheenRoughness;
	surfDef.sheenAmount = rgb.sheenAmount;
	surfDef.flags = rgb.flags;
	return surfDef;
}

bool isTwoSided(uint flags)
{
	return (flags & MaterialMask_TwoSided) != 0;
}

bool hasDispersion(uint flags)
{
	return (flags & MaterialMask_Dispersion) != 0;
}

SHADERTABLE_EXTRADATA_DECLARE(RayHitShaderTableConstantData);

float bbLoadFloat(in uint bufferIndex, in uint bufferOffset)
{
	return asfloat(g_buffersUint[NonUniformResourceIndex(bufferIndex)][bufferOffset]);
}

float2 bbLoadFloat2(in uint bufferIndex, in uint bufferOffset)
{
	float2 retVal;
	retVal.x = bbLoadFloat(bufferIndex, bufferOffset);
	retVal.y = bbLoadFloat(bufferIndex, bufferOffset + 1);
	return retVal;
}

float3 bbLoadFloat3(in uint bufferIndex, in uint bufferOffset)
{
	float3 retVal;
	retVal.xy = bbLoadFloat2(bufferIndex, bufferOffset);
	retVal.z = bbLoadFloat(bufferIndex, bufferOffset + 2);
	return retVal;
}

float4 bbLoadFloat4(in uint bufferIndex, in uint bufferOffset)
{
	float4 retVal;
	retVal.xy = bbLoadFloat2(bufferIndex, bufferOffset);
	retVal.zw = bbLoadFloat2(bufferIndex, bufferOffset + 2);
	return retVal;
}

//stride and offset are sizes in dwords (uint32/float32)
void unpackBufferInfo(in uint2 val, out uint bufferIndex, out uint bufferStride, out uint bufferOffset)
{
	bufferIndex = val.y & 0xFFFF;
	bufferStride = val.y >> 16;
	bufferOffset = val.x;
}

float2 unpackTextureTransformScale(uint scale)
{
	return float2(f16tof32(scale & 0xFFFF), f16tof32(scale >> 16));
}

bool isValidPackedBufferInfo(in uint2 val)
{
	return val.x != uint(-1) && val.y != uint(-1);
}

uint3 fetchIndices()
{

	uint indexBufferIndex;
	uint indexBufferStridePacked;
	uint indexBufferOffset;
	
	unpackBufferInfo(SHADERTABLE_EXTRADATA.indexBuffer, indexBufferIndex, indexBufferStridePacked, indexBufferOffset);
	
	uint extraOffset = indexBufferStridePacked >> 8;
	uint indexBufferStride = indexBufferStridePacked & 0xFF;

	uint primIndex = PrimitiveIndex();

	uint3 indices;

	if (indexBufferStride == 2) //stride of 2 means 16 bit indices
	{
		int ind = (primIndex / 2) * 3;
		int odd = primIndex & 0x1;

		if (extraOffset > 0)
		{
			if (odd != 0)
			{
				ind += 1;
				odd = 0;
			}
			else
			{
				odd += 1;
			}
		}

		if (odd != 0)
		{
			uint val0 = g_buffersUint[NonUniformResourceIndex(indexBufferIndex)][ind + indexBufferOffset + 1];
			uint val1 = g_buffersUint[NonUniformResourceIndex(indexBufferIndex)][ind + indexBufferOffset + 2];

			indices.x = val0 >> 16;
			indices.y = val1 & 0xFFFF;
			indices.z = val1 >> 16;
		}
		else
		{
			uint val0 = g_buffersUint[NonUniformResourceIndex(indexBufferIndex)][ind + indexBufferOffset];
			uint val1 = g_buffersUint[NonUniformResourceIndex(indexBufferIndex)][ind + indexBufferOffset + 1];

			indices.x = val0 & 0xFFFF;
			indices.y = val0 >> 16;
			indices.z = val1 & 0xFFFF;
		}

		

	}
	else
	{
		indices.x = g_buffersUint[NonUniformResourceIndex(indexBufferIndex)][primIndex * indexBufferStride + indexBufferOffset];
		indices.y = g_buffersUint[NonUniformResourceIndex(indexBufferIndex)][primIndex * indexBufferStride + indexBufferOffset + 1];
		indices.z = g_buffersUint[NonUniformResourceIndex(indexBufferIndex)][primIndex * indexBufferStride + indexBufferOffset + 2];
	}

	


	return indices;
}

float3 fetchMeshNormal(in uint3 indices, in float3 barycentrics)
{
	uint bufferIndex;
	uint bufferStride;
	uint bufferOffset;
	
	unpackBufferInfo(SHADERTABLE_EXTRADATA.normalBuffer, bufferIndex, bufferStride, bufferOffset);
	
	float3 n1 = bbLoadFloat3(bufferIndex, bufferOffset + indices.x * bufferStride);
	float3 n2 = bbLoadFloat3(bufferIndex, bufferOffset + indices.y * bufferStride);
	float3 n3 = bbLoadFloat3(bufferIndex, bufferOffset + indices.z * bufferStride);

	return normalize(barycentrics.x * n1 + barycentrics.y * n2 + barycentrics.z * n3);
}

bool meshHasValidTangents()
{
	return isValidPackedBufferInfo(SHADERTABLE_EXTRADATA.tangentBuffer);
}

float3 fetchMeshTangent(in uint3 indices, in float3 barycentrics)
{
	uint bufferIndex;
	uint bufferStride;
	uint bufferOffset;
	
	unpackBufferInfo(SHADERTABLE_EXTRADATA.tangentBuffer, bufferIndex, bufferStride, bufferOffset);
	
	float4 t1 = bbLoadFloat4(bufferIndex, bufferOffset + indices.x * bufferStride);
	float4 t2 = bbLoadFloat4(bufferIndex, bufferOffset + indices.y * bufferStride);
	float4 t3 = bbLoadFloat4(bufferIndex, bufferOffset + indices.z * bufferStride);

	t1.xyz *= t1.w;
	t2.xyz *= t2.w;
	t3.xyz *= t3.w;

	return normalize(barycentrics.x * t1.xyz + barycentrics.y * t2.xyz + barycentrics.z * t3.xyz);
}

bool meshHasValidUVs()
{
	return isValidPackedBufferInfo(SHADERTABLE_EXTRADATA.uvBuffer);
}

float2 fetchMeshUV(in uint3 indices, in float3 barycentrics)
{
	uint bufferIndex;
	uint bufferStride;
	uint bufferOffset;
	
	unpackBufferInfo(SHADERTABLE_EXTRADATA.uvBuffer, bufferIndex, bufferStride, bufferOffset);
	
	float2 uv1 = bbLoadFloat2(bufferIndex, bufferOffset + indices.x * bufferStride);
	float2 uv2 = bbLoadFloat2(bufferIndex, bufferOffset + indices.y * bufferStride);
	float2 uv3 = bbLoadFloat2(bufferIndex, bufferOffset + indices.z * bufferStride);

	return barycentrics.x * uv1 + barycentrics.y * uv2 + barycentrics.z * uv3;
}

void makeOrthogonal(in float3 n, inout float3 t)
{
	n = normalize(t - n*(dot(n, t)));
	
}


void fetchSurfaceMaterialParameters(inout SurfaceDefinitionRGB surfaceDef)
{
	surfaceDef.albedo = SHADERTABLE_EXTRADATA.albedoTransparency.xyz;
	surfaceDef.transparency = SHADERTABLE_EXTRADATA.albedoTransparency.a;
	surfaceDef.specular = SHADERTABLE_EXTRADATA.specularMetalness.xyz;
	surfaceDef.metalness = SHADERTABLE_EXTRADATA.specularMetalness.a;
	surfaceDef.absorption = SHADERTABLE_EXTRADATA.absorptionDielectricIOR.xyz;
	surfaceDef.dielectricIOR = SHADERTABLE_EXTRADATA.absorptionDielectricIOR.a;
	surfaceDef.emissive = SHADERTABLE_EXTRADATA.emissiveRoughness.xyz;
	surfaceDef.roughness = SHADERTABLE_EXTRADATA.emissiveRoughness.a;
	surfaceDef.specularAmount = SHADERTABLE_EXTRADATA.specAmountClearCoatAmountIORRoughness.x;
	surfaceDef.clearCoatAmount = SHADERTABLE_EXTRADATA.specAmountClearCoatAmountIORRoughness.y;
	surfaceDef.clearCoatIOR = SHADERTABLE_EXTRADATA.specAmountClearCoatAmountIORRoughness.z;
	surfaceDef.clearCoatRoughness = SHADERTABLE_EXTRADATA.specAmountClearCoatAmountIORRoughness.w;
	surfaceDef.cauchysCoeffs = SHADERTABLE_EXTRADATA.cauchysCoefficients.xy;

	surfaceDef.anisotropy = SHADERTABLE_EXTRADATA.anisotropy;
	surfaceDef.anisotropyRotation = SHADERTABLE_EXTRADATA.anisotropyRotation;
	surfaceDef.thinFilmThickness = SHADERTABLE_EXTRADATA.thinFilmThickness;
	surfaceDef.flags = SHADERTABLE_EXTRADATA.materialMask;
	
	surfaceDef.sheenColor = SHADERTABLE_EXTRADATA.sheenColorRoughness.rgb;
	surfaceDef.sheenRoughness = SHADERTABLE_EXTRADATA.sheenColorRoughness.a;
	surfaceDef.sheenAmount = SHADERTABLE_EXTRADATA.sheenAmount;
	
	surfaceDef.sheenRoughness = max(surfaceDef.sheenRoughness, 0.07f); //minimum sheen roughness is 0.07

}

void modifySurfaceMaterialParametersWithTextures(in float2 uv, inout float3 normal, inout float3 tangent, inout SurfaceDefinitionRGB surfaceDef)
{

	if(SHADERTABLE_EXTRADATA.albedoTexIndex.x != TEX_UNBOUND_INDEX)
	{
		float2 uvScale = unpackTextureTransformScale(SHADERTABLE_EXTRADATA.albedoTexIndex.y);

		float4 atex = g_textures2D[SHADERTABLE_EXTRADATA.albedoTexIndex.x].SampleLevel(g_colorSampler, uv * uvScale, 0);
		surfaceDef.albedo *= atex.rgb;
	}
	
	if(SHADERTABLE_EXTRADATA.normalTexIndex.x != TEX_UNBOUND_INDEX) //TODO
	{
		float2 uvScale = unpackTextureTransformScale(SHADERTABLE_EXTRADATA.normalTexIndex.y);
		float4 n = g_textures2D[SHADERTABLE_EXTRADATA.normalTexIndex.x].SampleLevel(g_colorSampler, uv * uvScale, 0);
		//surfaceDef.albedo = atex.rgb;
	}
	
	if(SHADERTABLE_EXTRADATA.ormTexIndex.x != TEX_UNBOUND_INDEX)
	{
		float2 uvScale = unpackTextureTransformScale(SHADERTABLE_EXTRADATA.ormTexIndex.y);
		float4 orm = g_textures2D[SHADERTABLE_EXTRADATA.ormTexIndex.x].SampleLevel(g_colorSampler, uv * uvScale, 0);
		surfaceDef.roughness = orm.x;
		surfaceDef.roughness = orm.y;
		surfaceDef.metalness = orm.z;
	}
	
	if(SHADERTABLE_EXTRADATA.emissiveTexIndex.x != TEX_UNBOUND_INDEX)
	{
		float2 uvScale = unpackTextureTransformScale(SHADERTABLE_EXTRADATA.emissiveTexIndex.y);
		float4 emissive = g_textures2D[SHADERTABLE_EXTRADATA.emissiveTexIndex.x].SampleLevel(g_colorSampler, uv * uvScale, 0);
		surfaceDef.emissive *= emissive.rgb;
	}
	
}

float getRefractiveIndexForWavelength(float2 cauchysCoeffs, float waveLength)
{
	return cauchysCoeffs.x + (cauchysCoeffs.y / (waveLength * waveLength));
}

SpectralSamples calculateTransmittance(float distance, SpectralSamples absorption)
{
	SpectralSamples s;
	for (uint i = 0; i < absorption.getSampleCount(); ++i)
	{
		float v = exp(-absorption[i] * distance);
		s.setInd(i, v);
	}
	return s;
}

#endif