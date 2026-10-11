#ifndef DENOISE_FEATURES_HLSL_INCL
#define DENOISE_FEATURES_HLSL_INCL

#include "materialParameterTextures.hlsl"

//Denoise features are accumulated over every sample of the pixel the same way the radiance is (so defocused / motion blurred features
//blur like the image does), along with second moments so that per pixel feature variance can be used to relax the edge stopping weights
//("Robust Denoising using Feature and Color Information", Rousselle et al. 2013)

//accumulated feature moments, running means over all the samples of the pixel (samples that miss contribute zeros)
//layer 0: albedo.rgb, |albedo|^2
//layer 1: normal.xyz, coverage (fraction of samples that hit geometry)
//layer 2: roughness, metalness, transparency, anisotropy
//layer 3: ior * FEATURE_IOR_SCALE, |material|^2, depth * hit, depth^2 * hit
#define FEATURE_ACCUM_LAYER_COUNT 4
#define FEATURE_IOR_SCALE 0.5f //lessen the effect of ior on the material distance
#define FEATURE_HALF_MAX 65000.f

struct FeatureMoments
{
	float4 layer0;
	float4 layer1;
	float4 layer2;
	float4 layer3;
};

FeatureMoments featureSampleToMoments(MaterialParameters2Texture f)
{
	FeatureMoments m = (FeatureMoments) 0;
	if ((f.materialFlags & MATERIAL_PARAMS_FLAG_HIT) == 0)
	{
		return m;
	}

	float4 mat = float4(f.roughness, f.metalness, f.transparency, f.anisotropy);
	float ior = f.ior * FEATURE_IOR_SCALE;

	m.layer0 = float4(f.albedo, dot(f.albedo, f.albedo));
	m.layer1 = float4(f.normal, 1.f);
	m.layer2 = mat;
	m.layer3 = float4(ior, dot(mat, mat) + ior * ior, f.depth, f.depth * f.depth);
	return m;
}

//filtered per pixel features used for the edge stopping weights
struct DenoiseFeatures
{
	float3 albedo;
	float3 normal; //mean normal, not normalized: its length shrinks as the normals within the pixel disagree
	float depth; //planar view space depth, mean over the samples that hit geometry, 0 when none did
	float roughness;
	float metalness;
	float transparency;
	float anisotropy;
	float ior; //scaled by FEATURE_IOR_SCALE
	float varAlbedo;
	float varDepth;
	float varMaterial;
};

DenoiseFeatures featuresFromMoments(FeatureMoments m)
{
	DenoiseFeatures f;
	f.albedo = m.layer0.xyz;
	f.varAlbedo = max(m.layer0.w - dot(f.albedo, f.albedo), 0.f);

	f.normal = m.layer1.xyz;
	float coverage = m.layer1.w;

	f.roughness = m.layer2.x;
	f.metalness = m.layer2.y;
	f.transparency = m.layer2.z;
	f.anisotropy = m.layer2.w;
	f.ior = m.layer3.x;
	f.varMaterial = max(m.layer3.y - dot(m.layer2, m.layer2) - f.ior * f.ior, 0.f);

	if (coverage > 0.f)
	{
		f.depth = m.layer3.z / coverage;
		f.varDepth = max(m.layer3.w / coverage - f.depth * f.depth, 0.f);
	}
	else
	{
		f.depth = 0.f;
		f.varDepth = 0.f;
	}
	return f;
}

float getNormalVariance(DenoiseFeatures f)
{
	return saturate(1.f - length(f.normal));
}

float4 getMaterialVector(DenoiseFeatures f)
{
	return float4(f.roughness, f.metalness, f.transparency, f.anisotropy);
}

uint packHalf2(float a, float b)
{
	return f32tof16(a) | (f32tof16(b) << 16);
}

void writeDenoiseFeatures(uint2 location, DenoiseFeatures f, RWTexture2D<uint4> outputTex0, RWTexture2D<uint4> outputTex1)
{
	//depth variance is stored relative to depth^2 to keep it in half range
	float relVarDepth = f.depth > 0.f ? min(f.varDepth / (f.depth * f.depth), FEATURE_HALF_MAX) : 0.f;

	outputTex0[location] = uint4(
		packHalf2(f.albedo.x, f.albedo.y),
		packHalf2(f.albedo.z, relVarDepth),
		packHalf2(f.normal.x, f.normal.y),
		packHalf2(f.normal.z, f.roughness));

	outputTex1[location] = uint4(
		asuint(f.depth),
		packHalf2(f.metalness, f.anisotropy),
		packHalf2(f.transparency, f.ior),
		packHalf2(min(f.varAlbedo, FEATURE_HALF_MAX), min(f.varMaterial, FEATURE_HALF_MAX)));
}

DenoiseFeatures readDenoiseFeatures(uint2 location, Texture2D<uint4> inputTex0, Texture2D<uint4> inputTex1)
{
	uint4 v0 = inputTex0[location];
	uint4 v1 = inputTex1[location];

	DenoiseFeatures f;
	f.albedo = float3(f16tof32(v0.x & 0xFFFF), f16tof32(v0.x >> 16), f16tof32(v0.y & 0xFFFF));
	float relVarDepth = f16tof32(v0.y >> 16);
	f.normal = float3(f16tof32(v0.z & 0xFFFF), f16tof32(v0.z >> 16), f16tof32(v0.w & 0xFFFF));
	f.roughness = f16tof32(v0.w >> 16);

	f.depth = asfloat(v1.x);
	f.metalness = f16tof32(v1.y & 0xFFFF);
	f.anisotropy = f16tof32(v1.y >> 16);
	f.transparency = f16tof32(v1.z & 0xFFFF);
	f.ior = f16tof32(v1.z >> 16);
	f.varAlbedo = f16tof32(v1.w & 0xFFFF);
	f.varMaterial = f16tof32(v1.w >> 16);

	f.varDepth = relVarDepth * f.depth * f.depth;
	return f;
}

struct FeatureWeightParams
{
	float normalSigma;
	float materialSigma;
	float depthSigmaScale;
	float varianceScale;
};

//w = exp(-|f_p - f_q|^2 / (sigma^2 + k * (Var_p + Var_q))): in-focus pixels have ~zero feature variance and keep sharp edges,
//defocused ones have high variance and relax automatically
float calculateVarianceNormalizedWeight(float distSqr, float sigma, float varianceSum, float varianceScale)
{
	return exp(-distSqr / (sigma * sigma + varianceScale * varianceSum + 1e-8f));
}

//normal, albedo and material terms (depth is handled separately as the callers measure distance differently)
float calculateFeatureWeight(DenoiseFeatures a, DenoiseFeatures b, FeatureWeightParams params)
{
	float3 normalDelta = a.normal - b.normal;
	float wNormal = calculateVarianceNormalizedWeight(dot(normalDelta, normalDelta), params.normalSigma, getNormalVariance(a) + getNormalVariance(b), params.varianceScale);

	float3 albedoDelta = a.albedo - b.albedo;
	float wAlbedo = calculateVarianceNormalizedWeight(dot(albedoDelta, albedoDelta), params.materialSigma, a.varAlbedo + b.varAlbedo, params.varianceScale);

	float4 matDelta = getMaterialVector(a) - getMaterialVector(b);
	float iorDelta = a.ior - b.ior;
	float wMat = calculateVarianceNormalizedWeight(dot(matDelta, matDelta) + iorDelta * iorDelta, params.materialSigma, a.varMaterial + b.varMaterial, params.varianceScale);

	return wNormal * wAlbedo * wMat;
}

#endif
