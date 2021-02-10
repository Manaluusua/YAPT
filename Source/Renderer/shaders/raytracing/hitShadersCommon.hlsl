#ifndef HITSHADERSCOMMON_HLSL_INCL
#define HITSHADERSCOMMON_HLSL_INCL

#include "raytraceCommonResources.hlsl"

#define MaterialMask_TwoSided (0x1)
#define TEX_UNBOUND_INDEX (~0)


struct SurfaceDefinition
{
	float3x3 toCoatingLayerTangentSpace;
	float3x3 toBaseLayerTangentSpace;
	
	float3 geometryNormal;
	
	float3 albedo;
	float transparency;
	float3 specular;
	float metalness;
	float3 absorption;
	float dielectricIOR;
	float3 emissive;
	float roughness;
	

	float specularAmount;
	float clearCoatAmount;
	float clearCoatIOR;
	float clearCoatRoughness;

	float anisotropy;
	float anisotropyRotation;
	float thinFilmThickness;
	float pad;
	
	float3 sheenColor;
	float sheenRoughness;
	
	float sheenAmount;
	bool isTwoSided;
};

float bbLoadFloat(in uint bufferIndex, in uint bufferOffset)
{
	return (g_buffersFloat[NonUniformResourceIndex(bufferIndex)][bufferOffset]);
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

uint3 fetchIndices(uint2 indexBuffer)
{
	uint primIndex = PrimitiveIndex();
	
	uint indexBufferIndex;
	uint indexBufferStride;
	uint indexBufferOffset;
	
	unpackBufferInfo(indexBuffer, indexBufferIndex, indexBufferStride, indexBufferOffset);
	
	uint3 indices;

	indices.x = g_buffersUint[NonUniformResourceIndex(indexBufferIndex)][primIndex * indexBufferStride + indexBufferOffset];
	indices.y = g_buffersUint[NonUniformResourceIndex(indexBufferIndex)][primIndex * indexBufferStride + indexBufferOffset + 1];
	indices.z = g_buffersUint[NonUniformResourceIndex(indexBufferIndex)][primIndex * indexBufferStride + indexBufferOffset + 2];


	return indices;
}

float3 fetchMeshNormal(in uint2 normalBuffer, in uint3 indices, in float3 barycentrics)
{
	uint bufferIndex;
	uint bufferStride;
	uint bufferOffset;
	
	unpackBufferInfo(normalBuffer, bufferIndex, bufferStride, bufferOffset);
	
	float3 n1 = bbLoadFloat3(bufferIndex, bufferOffset + indices.x * bufferStride);
	float3 n2 = bbLoadFloat3(bufferIndex, bufferOffset + indices.y * bufferStride);
	float3 n3 = bbLoadFloat3(bufferIndex, bufferOffset + indices.z * bufferStride);

	return normalize(barycentrics.x * n1 + barycentrics.y * n2 + barycentrics.z * n3);
}

float3 fetchMeshTangent(in uint2 tangentBuffer, in uint3 indices, in float3 barycentrics)
{
	uint bufferIndex;
	uint bufferStride;
	uint bufferOffset;
	
	unpackBufferInfo(tangentBuffer, bufferIndex, bufferStride, bufferOffset);
	
	float4 t1 = bbLoadFloat4(bufferIndex, bufferOffset + indices.x * bufferStride);
	float4 t2 = bbLoadFloat4(bufferIndex, bufferOffset + indices.y * bufferStride);
	float4 t3 = bbLoadFloat4(bufferIndex, bufferOffset + indices.z * bufferStride);

	t1.xyz *= t1.w;
	t2.xyz *= t2.w;
	t3.xyz *= t3.w;

	return normalize(barycentrics.x * t1.xyz + barycentrics.y * t2.xyz + barycentrics.z * t3.xyz);
}

float2 fetchMeshUV(in uint2 uvBuffer, in uint3 indices, in float3 barycentrics)
{
	uint bufferIndex;
	uint bufferStride;
	uint bufferOffset;
	
	unpackBufferInfo(uvBuffer, bufferIndex, bufferStride, bufferOffset);
	
	float2 uv1 = bbLoadFloat2(bufferIndex, bufferOffset + indices.x * bufferStride);
	float2 uv2 = bbLoadFloat2(bufferIndex, bufferOffset + indices.y * bufferStride);
	float2 uv3 = bbLoadFloat2(bufferIndex, bufferOffset + indices.z * bufferStride);

	return barycentrics.x * uv1 + barycentrics.y * uv2 + barycentrics.z * uv3;
}

void makeOrthogonal(in float3 n, inout float3 t)
{
	n = normalize(t - n*(dot(n, t)));
	
}


void fetchSurfaceMaterialParameters(in ObjectData objectData, inout SurfaceDefinition surfaceDef)
{
	surfaceDef.albedo = objectData.albedoTransparency.xyz;
	surfaceDef.transparency = objectData.albedoTransparency.a;
	surfaceDef.specular = objectData.specularMetalness.xyz;
	surfaceDef.metalness = objectData.specularMetalness.a;
	surfaceDef.absorption = objectData.absorptionDielectricIOR.xyz;
	surfaceDef.dielectricIOR = objectData.absorptionDielectricIOR.a;
	surfaceDef.emissive = objectData.emissiveRoughness.xyz;
	surfaceDef.roughness = objectData.emissiveRoughness.a;
	surfaceDef.specularAmount = objectData.specAmountClearCoatAmountIORRoughness.x;
	surfaceDef.clearCoatAmount = objectData.specAmountClearCoatAmountIORRoughness.y;
	surfaceDef.clearCoatIOR = objectData.specAmountClearCoatAmountIORRoughness.z;
	surfaceDef.clearCoatRoughness = objectData.specAmountClearCoatAmountIORRoughness.w;

	surfaceDef.anisotropy = objectData.anisotropy;
	surfaceDef.anisotropyRotation = objectData.anisotropyRotation;
	surfaceDef.thinFilmThickness = objectData.thinFilmThickness;
	surfaceDef.isTwoSided = (objectData.materialMask & MaterialMask_TwoSided) != 0;
	
	surfaceDef.sheenColor = objectData.sheenColorRoughness.rgb;
	surfaceDef.sheenRoughness = objectData.sheenColorRoughness.a;
	surfaceDef.sheenAmount = objectData.sheenAmount;
	
	surfaceDef.sheenRoughness = max(surfaceDef.sheenRoughness, 0.07f); //minimum sheen roughness is 0.07

}

void modifySurfaceMaterialParametersWithTextures(in uint albedoTexIndex, in uint normalTexIndex, in uint ormTexIndex, in uint emissiveTexIndex, in float2 uv, 
	inout float3 normal, inout float3 tangent, inout SurfaceDefinition surfaceDef)
{
	
	if(albedoTexIndex != TEX_UNBOUND_INDEX)
	{
		float4 atex = g_textures2D[NonUniformResourceIndex(albedoTexIndex)].SampleLevel(g_colorSampler, uv, 0);
		surfaceDef.albedo *= atex.rgb;
	}
	
	if(normalTexIndex != TEX_UNBOUND_INDEX)
	{
		float4 n = g_textures2D[NonUniformResourceIndex(normalTexIndex)].SampleLevel(g_colorSampler, uv, 0);
		//surfaceDef.albedo = atex.rgb;
	}
	
	if(ormTexIndex != TEX_UNBOUND_INDEX)
	{
		float4 orm = g_textures2D[NonUniformResourceIndex(ormTexIndex)].SampleLevel(g_colorSampler, uv, 0);
		//surfaceDef.roughness = orm.x;
		surfaceDef.roughness = orm.y;
		surfaceDef.metalness = orm.z;
	}
	
	if(emissiveTexIndex != TEX_UNBOUND_INDEX)
	{
		float4 emissive = g_textures2D[NonUniformResourceIndex(emissiveTexIndex)].SampleLevel(g_colorSampler, uv, 0);
		surfaceDef.emissive *= emissive.rgb;
	}
	
}
	
#endif