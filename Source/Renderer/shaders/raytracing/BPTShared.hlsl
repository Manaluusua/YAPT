#ifndef BPT_SHARED_HLSL_INCL
#define BPT_SHARED_HLSL_INCL

RaytracingAccelerationStructure g_accelerationStructure : register(t0, space3);
RWTexture2D<float4> g_outputColor : register(u1, space3);
RWTexture2D<uint4> g_MaterialParamsOutput0 : register(u2, space3);
RWTexture2D<uint4> g_MaterialParamsOutput1 : register(u3, space3);

static float g_spectralMainSampleWavelength;
#define GET_SPECTRAL_SAMPLE_WAVELENGTH g_spectralMainSampleWavelength;

#endif