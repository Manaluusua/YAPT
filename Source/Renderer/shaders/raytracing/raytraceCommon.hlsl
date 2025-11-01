#ifndef RAYTRACE_COMMON_HLSL_INCL
#define RAYTRACE_COMMON_HLSL_INCL

#include "../globalDefinitions.hlsl"
#include "../common/commonMath.hlsl"

//keep in sync with SpectralData.h
#include "../sharedIncludes/spectralConstants.h"

//keep in sync with raytrace stage
#define NUMBER_OF_RANDOM_SAMPLES 1u
#define NUMBER_OF_RANDOM_SAMPLE_DIMENSIONS 256u

#define RAY_STATE_ALIVE 0
#define RAY_STATE_TERMINATED 1
#define RAY_STATE_CANCELLED 2
#define RAY_MAX_VOLUMES_ENTERED 4

#define IOR_DEFAULT (1.0f)

#define DEFAULT_RAY_MIN_T (0.001f)
#define DEFAULT_RAY_MAX_T (1.0f / 0.0f)

//#define WHITE_FURNACE_TEST
//#define WHITE_FURNACE_TEST_BOUNCE_LIMIT 64

#endif