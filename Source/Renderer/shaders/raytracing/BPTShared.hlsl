#ifndef BPT_SHARED_HLSL_INCL
#define BPT_SHARED_HLSL_INCL

#define RTCR_SPECTRAL_SAMPLESET_EXTRA_OFFSET ((DispatchRaysIndex().y % 2) * 2 + (DispatchRaysIndex().x % 2))

#endif