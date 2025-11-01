#ifndef HITSHADERSCOMMON_HLSL_INCL
#define HITSHADERSCOMMON_HLSL_INCL

#include "raytraceCommonResources.hlsl"

#define MaterialMask_TwoSided (1 << 0)
#define MaterialMask_Dispersion (1 << 1)
#define TEX_UNBOUND_INDEX (~0)

struct SurfaceDefinitionRGB
{
    float3 coatingLayerNormal;
    float specularAmount;
    float3 baseLayerNormal;
    float clearCoatAmount;
    float3 geometryNormal;
    float clearCoatIOR;
    float3 tangent;
    float clearCoatRoughness;

	float3 specular;
	float metalness;
	float3 albedo;
	float transparency;
	float3 absorption;
	float dielectricIOR;
	float3 emissive;
	float roughness;
	
    float2 cauchysCoeffs;
	float anisotropy;
	float anisotropyRotation;

	float3 sheenColor;
	float sheenRoughness;
	
    float thinFilmThickness;
	float sheenAmount;
	uint flags;
};

struct SurfaceDefinition
{
    SpectralSamples specular;
    SpectralSamples albedo;
    SpectralSamples absorption;
    SpectralSamples emissive;
    SpectralSamples sheenColor;
	
    float3 coatingLayerNormal;
    float transparency;
    float3 baseLayerNormal;
    float metalness;
    float3 geometryNormal;
    float dielectricIOR;
    float3 tangent;
    float roughness;

    float2 cauchysCoeffs;
	float specularAmount;
	float clearCoatAmount;
	
	float clearCoatIOR;
	float clearCoatRoughness;
	float anisotropy;
	float anisotropyRotation;
	
	float thinFilmThickness;
	float sheenRoughness;
	float sheenAmount;
	uint flags;
};



SurfaceDefinition convertSurfaceDefinitionFromRGB(SurfaceDefinitionRGB rgb)
{
	SurfaceDefinition surfDef;
    surfDef.coatingLayerNormal = rgb.coatingLayerNormal;
    surfDef.baseLayerNormal = rgb.baseLayerNormal;
    surfDef.geometryNormal = rgb.geometryNormal;
    surfDef.tangent = rgb.tangent;

	surfDef.specular.setFromRGB(rgb.specular);
	surfDef.albedo.setFromRGB(rgb.albedo);
	surfDef.absorption.setFromRGB(rgb.absorption);
	surfDef.emissive.setFromRGBUnbounded(rgb.emissive);
	surfDef.sheenColor.setFromRGB(rgb.sheenColor);

	

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

uint3 fetchIndices(uint2 indexBuffer, uint primIndex)
{

	uint indexBufferIndex;
	uint indexBufferStridePacked;
	uint indexBufferOffset;
	
	unpackBufferInfo(indexBuffer, indexBufferIndex, indexBufferStridePacked, indexBufferOffset);
	
	uint extraOffset = indexBufferStridePacked >> 8;
	uint indexBufferStride = indexBufferStridePacked & 0xFF;

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

void fetchMeshPositions(in uint2 posBuffer, in uint3 indices, out float3 p1, out float3 p2, out float3 p3)
{
	uint bufferIndex;
	uint bufferStride;
	uint bufferOffset;

	unpackBufferInfo(posBuffer, bufferIndex, bufferStride, bufferOffset);

	p1 = bbLoadFloat3(bufferIndex, bufferOffset + indices.x * bufferStride);
	p2 = bbLoadFloat3(bufferIndex, bufferOffset + indices.y * bufferStride);
	p3 = bbLoadFloat3(bufferIndex, bufferOffset + indices.z * bufferStride);
}

float3 fetchMeshPosition(in uint2 posBuffer, in uint3 indices, in float3 barycentrics)
{
	
	float3 p1;
	float3 p2;
	float3 p3;
	fetchMeshPositions(posBuffer, indices, p1, p2, p3);
	return barycentrics.x * p1 + barycentrics.y * p2 + barycentrics.z * p3;
}

void fetchMeshNormals(in uint2 normalBuffer, in uint3 indices, out float3 n1, out float3 n2, out float3 n3)
{
	uint bufferIndex;
	uint bufferStride;
	uint bufferOffset;

	unpackBufferInfo(normalBuffer, bufferIndex, bufferStride, bufferOffset);

	n1 = bbLoadFloat3(bufferIndex, bufferOffset + indices.x * bufferStride);
	n2 = bbLoadFloat3(bufferIndex, bufferOffset + indices.y * bufferStride);
	n3 = bbLoadFloat3(bufferIndex, bufferOffset + indices.z * bufferStride);

}

float3 fetchMeshNormal(in uint2 normalBuffer, in uint3 indices, in float3 barycentrics)
{
	float3 n1;
	float3 n2;
	float3 n3;

	fetchMeshNormals(normalBuffer, indices, n1, n2, n3);

	return normalize(barycentrics.x * n1 + barycentrics.y * n2 + barycentrics.z * n3);
}

float3 fetchMeshTriangleNormal(in uint2 posBuffer, in uint3 indices, in float3 barycentrics)
{
    float3 p1;
    float3 p2;
    float3 p3;
    fetchMeshPositions(posBuffer, indices, p1, p2, p3);

    return normalize(cross(p2 - p1, p3 - p1));
}

bool meshHasValidTangents(in uint2 tangentBuffer)
{
	return isValidPackedBufferInfo(tangentBuffer);
}

void fetchMeshTangents(in uint2 tangentBuffer, in uint3 indices, out float3 t1, out float3 t2, out float3 t3)
{
	uint bufferIndex;
	uint bufferStride;
	uint bufferOffset;

	unpackBufferInfo(tangentBuffer, bufferIndex, bufferStride, bufferOffset);

	float4 tan1 = bbLoadFloat4(bufferIndex, bufferOffset + indices.x * bufferStride);
	float4 tan2 = bbLoadFloat4(bufferIndex, bufferOffset + indices.y * bufferStride);
	float4 tan3 = bbLoadFloat4(bufferIndex, bufferOffset + indices.z * bufferStride);

	t1 = tan1.xyz * tan1.w;
	t2 = tan2.xyz * tan2.w;
	t3 = tan3.xyz * tan3.w;
}

float3 fetchMeshTangent(in uint2 tangentBuffer, in uint3 indices, in float3 barycentrics)
{
	float3 t1;
	float3 t2;
	float3 t3;
	fetchMeshTangents(tangentBuffer, indices, t1, t2, t3);

	return normalize(barycentrics.x * t1.xyz + barycentrics.y * t2.xyz + barycentrics.z * t3.xyz);
}

bool meshHasValidUVs(in uint2 uvBuffer)
{
	return isValidPackedBufferInfo(uvBuffer);
}

void fetchMeshUVs(in uint2 uvBuffer, in uint3 indices, out float2 uv1, out float2 uv2, out float2 uv3)
{
	uint bufferIndex;
	uint bufferStride;
	uint bufferOffset;

	unpackBufferInfo(uvBuffer, bufferIndex, bufferStride, bufferOffset);

	uv1 = bbLoadFloat2(bufferIndex, bufferOffset + indices.x * bufferStride);
	uv2 = bbLoadFloat2(bufferIndex, bufferOffset + indices.y * bufferStride);
	uv3 = bbLoadFloat2(bufferIndex, bufferOffset + indices.z * bufferStride);
}

float2 fetchMeshUV(in uint2 uvBuffer, in uint3 indices, in float3 barycentrics)
{

	float2 uv1;
	float2 uv2;
	float2 uv3;
	fetchMeshUVs(uvBuffer, indices, uv1, uv2, uv3);
	return barycentrics.x * uv1 + barycentrics.y * uv2 + barycentrics.z * uv3;
}


void fetchSurfaceMaterialParameters(in MaterialEntryGPU matEntry, inout SurfaceDefinitionRGB surfaceDef)
{
	

	surfaceDef.albedo = matEntry.albedoTransparency.xyz;
	surfaceDef.transparency = matEntry.albedoTransparency.a;
	surfaceDef.specular = matEntry.specularMetalness.xyz;
	surfaceDef.metalness = matEntry.specularMetalness.a;
	surfaceDef.absorption = matEntry.absorptionDielectricIOR.xyz;
	surfaceDef.dielectricIOR = matEntry.absorptionDielectricIOR.a;
	surfaceDef.emissive = matEntry.emissiveRoughness.xyz;
	surfaceDef.roughness = matEntry.emissiveRoughness.a;
	surfaceDef.specularAmount = matEntry.specAmountClearCoatAmountIORRoughness.x;
	surfaceDef.clearCoatAmount = matEntry.specAmountClearCoatAmountIORRoughness.y;
	surfaceDef.clearCoatIOR = matEntry.specAmountClearCoatAmountIORRoughness.z;
	surfaceDef.clearCoatRoughness = matEntry.specAmountClearCoatAmountIORRoughness.w;
	surfaceDef.cauchysCoeffs = matEntry.cauchysCoefficients.xy;

	surfaceDef.anisotropy = matEntry.anisotropy;
	surfaceDef.anisotropyRotation = matEntry.anisotropyRotation;
	surfaceDef.thinFilmThickness = matEntry.thinFilmThickness;
	surfaceDef.flags = matEntry.materialMask;
	
	surfaceDef.sheenColor = matEntry.sheenColorRoughness.rgb;
	surfaceDef.sheenRoughness = matEntry.sheenColorRoughness.a;
	surfaceDef.sheenAmount = matEntry.sheenAmount;
	
	surfaceDef.sheenRoughness = max(surfaceDef.sheenRoughness, 0.07f); //minimum sheen roughness is 0.07

}

void modifySurfaceMaterialParametersWithTextures(in MaterialEntryGPU matEntry, in float2 uv, inout float3 normal, inout float3 tangent, inout SurfaceDefinitionRGB surfaceDef)
{

	if(matEntry.albedoTexIndexAndScale.x != TEX_UNBOUND_INDEX)
	{
		float2 uvScale = unpackTextureTransformScale(matEntry.albedoTexIndexAndScale.y);

		float4 atex = g_textures2D[matEntry.albedoTexIndexAndScale.x].SampleLevel(g_colorSampler, uv * uvScale, 0);
		surfaceDef.albedo *= atex.rgb;
	}
	
	if(matEntry.normalTexIndexAndScale.x != TEX_UNBOUND_INDEX) //TODO
	{
		float2 uvScale = unpackTextureTransformScale(matEntry.normalTexIndexAndScale.y);
		float4 n = g_textures2D[matEntry.normalTexIndexAndScale.x].SampleLevel(g_colorSampler, uv * uvScale, 0);
		//surfaceDef.albedo = atex.rgb;
	}
	
	if(matEntry.ormTexIndexAndScale.x != TEX_UNBOUND_INDEX)
	{
		float2 uvScale = unpackTextureTransformScale(matEntry.ormTexIndexAndScale.y);
		float4 orm = g_textures2D[matEntry.ormTexIndexAndScale.x].SampleLevel(g_colorSampler, uv * uvScale, 0);
		surfaceDef.roughness = orm.x;
		surfaceDef.roughness = orm.y;
		surfaceDef.metalness = orm.z;
	}
	
	if(matEntry.emissiveTexIndexAndScale.x != TEX_UNBOUND_INDEX)
	{
		float2 uvScale = unpackTextureTransformScale(matEntry.emissiveTexIndexAndScale.y);
		float4 emissive = g_textures2D[matEntry.emissiveTexIndexAndScale.x].SampleLevel(g_colorSampler, uv * uvScale, 0);
		surfaceDef.emissive *= emissive.rgb;
	}
	
}

void modifySurfaceEmissionWithTexture(in MaterialEntryGPU matEntry, in float2 uv, inout SurfaceDefinitionRGB surfaceDef)
{
    if (matEntry.emissiveTexIndexAndScale.x != TEX_UNBOUND_INDEX)
    {
        float2 uvScale = unpackTextureTransformScale(matEntry.emissiveTexIndexAndScale.y);
        float4 emissive = g_textures2D[matEntry.emissiveTexIndexAndScale.x].SampleLevel(g_colorSampler, uv * uvScale, 0);
        surfaceDef.emissive *= emissive.rgb;
    }
}

float3 nudgeNormal(float3 rayDir, float3 normal, float roughness, float transparency, bool twoSided)
{
	//if two sided, flip normal if view ray hitting from backside
	//when ray too orthogonal to a normal, nudge the normal (if transparent, nudge a bit more since (rough) transparency can generate very high peaks of energy from these cases) 
	{
        float rayDotN = dot(rayDir, normal);

        if (twoSided)
        {
            if (rayDotN > 0)
            {
                normal = -normal;
            }
        }

        float rayOrthogonalThreshold = 0.05f;
        float rayOrthogonalNudgeFactor = 0.05f;

        if (transparency != 0.f)
        {
            float RAY_ORTHOGONAL_THRESHOLD_MAX_ROUGHNESS = 0.2f;
            float RAY_ORTHOGONAL_NUDGE_FACTOR_MAX_ROUGHNESS = 0.2f;

            rayOrthogonalThreshold = lerp(rayOrthogonalThreshold, RAY_ORTHOGONAL_THRESHOLD_MAX_ROUGHNESS, roughness);
            rayOrthogonalNudgeFactor = lerp(rayOrthogonalNudgeFactor, RAY_ORTHOGONAL_NUDGE_FACTOR_MAX_ROUGHNESS, roughness);
        }

        if (abs(rayDotN) < rayOrthogonalThreshold)
        {
            normal = normalize(normal - rayDir * rayOrthogonalNudgeFactor);
        }


    }
    return normal;
}

void setupSurfaceOrientation(float3 geometryNormal, float3 normalBase, float3 normalCoating, float3 tangent, inout SurfaceDefinitionRGB surfaceDef)
{
	//add tangent space rotation (could later on optimize out the sin & cos by providing these precalculated on cpu)
    if (surfaceDef.anisotropyRotation > 0)
    {
        float rotAngle = surfaceDef.anisotropyRotation * 2 * PI;
        float cosA = cos(rotAngle);
        float sinA = sin(rotAngle);
        float3x3 rot = float3x3(cosA, 0, sinA,
								0, 1, 0,
								-sinA, 0, cosA);
        tangent = mul(rot, tangent);
		
    }

    surfaceDef.coatingLayerNormal = normalCoating;
    surfaceDef.baseLayerNormal = normalBase;
    surfaceDef.geometryNormal = geometryNormal;
    surfaceDef.tangent = tangent;

}

float getRefractiveIndexForWavelength(float2 cauchysCoeffs, float waveLengthNM)
{
    float waveLengthum = waveLengthNM * 0.001f;
    return cauchysCoeffs.x + (cauchysCoeffs.y / (waveLengthum * waveLengthum)); //assume cauchys coeffs are in micrometers
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

float3 getRaySpawnOffsetUsingNormal(float3 geometryNormal, float3 nextSampleDir)
{
    float offsetEpsilon = 0.001f;
    bool transmitted = dot(nextSampleDir, geometryNormal) < 0.f ? true : false;
		
    float3 rayOffset = geometryNormal * offsetEpsilon;
    rayOffset *= transmitted ? -1.f : 1.f;
    return rayOffset;
}

float3 getRaySpawnOffsetTowardsRay(float3 nextSampleDir)
{
    float offsetEpsilon = 0.001f;
    return nextSampleDir * offsetEpsilon;
}

#endif