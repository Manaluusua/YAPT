#ifndef SURFACE_PARAMETERS_HLSL_INCL
#define SURFACE_PARAMETERS_HLSL_INCL



float2 calculateRoughnessParams(float roughness, float anisotropy)
{
	float a2 = roughness * roughness;
	return clamp(float2(a2 * (1 + anisotropy),  a2 * (1 - anisotropy)), 0.001f, 1.f);
}


#endif