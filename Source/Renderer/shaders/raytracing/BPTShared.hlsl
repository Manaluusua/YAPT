#ifndef BPT_SHARED_HLSL_INCL
#define BPT_SHARED_HLSL_INCL

RaytracingAccelerationStructure g_accelerationStructure : register(t0, space3);
static float g_spectralMainSampleWavelength;
#define GET_SPECTRAL_SAMPLE_WAVELENGTH g_spectralMainSampleWavelength;

#endif