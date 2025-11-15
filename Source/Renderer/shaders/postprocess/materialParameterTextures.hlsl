#ifndef MATERIAL_PARAMETER_TEXTURES_HLSL_INCL
#define MATERIAL_PARAMETER_TEXTURES_HLSL_INCL

struct MaterialParameters2Texture
{
	float3 albedo;
	float3 normal;
	float metalness;
	float transparency;
	float depth;
	float roughness;
	float anisotropy;
	float ior;
	uint materialFlags;
};

void writeMaterialParametersToTextures(uint2 location,
	MaterialParameters2Texture params,
	RWTexture2D<uint4> outputTex0,
	RWTexture2D<uint4> outputTex1)
{
	uint4 v0 = uint4(
		f32tof16(params.albedo.x) | f32tof16 (params.albedo.y) << 16,
		f32tof16(params.albedo.z) | f32tof16 (params.transparency) << 16,
		f32tof16(params.normal.x) | f32tof16 (params.normal.y) << 16,
		f32tof16(params.normal.z) | f32tof16 (params.roughness) << 16
	);
	
	
	uint4 v1 = uint4(
		asuint(params.depth),
		f32tof16(params.metalness) | f32tof16 (params.anisotropy) << 16,
		f32tof16(params.ior) | f32tof16 (params.materialFlags) << 16,
		0
	);
	
	outputTex0[location] = v0;
	outputTex1[location] = v1;
}


MaterialParameters2Texture readMaterialParametersFromTextures(uint2 location,
	Texture2D<uint4> inputTex0,
	Texture2D<uint4> inputTex1)
{
	uint4 v0 = inputTex0[location];
	uint4 v1 = inputTex1[location];


	MaterialParameters2Texture output;
	output.albedo = float3(f16tof32(v0.x & 0xFFFF), f16tof32(v0.x >> 16), f16tof32(v0.y & 0xFFFF));
	output.transparency = v0.y >> 16;
	
	output.normal = float3(f16tof32(v0.z & 0xFFFF), f16tof32(v0.z >> 16), f16tof32(v0.w & 0xFFFF));
	output.roughness = v0.w >> 16;
	
	output.depth = asfloat(v1.x);
	output.metalness = v1.y & 0xFFFF;
	output.anisotropy = v1.y >> 16;
	output.ior = v1.z & 0xFFFF;
	output.materialFlags = v1.z >> 16;
	
	return output;
}

#endif