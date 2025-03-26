
#include "../globalDefinitions.hlsl"
#include "../utils/commonMath.hlsl"

//keep in sync with raytrace stage

#define NUMBER_OF_RANDOM_SAMPLES 256
#define SPECTRAL_SAMPLES_COUNT 3

//keep in sync with SpectralData.h
#define CIE_LUT_LAMBDA_MIN 360
#define CIE_LUT_LAMBDA_MAX 830
#define SIE_Y_SUM 106.8569171f;

#define RAY_STATE_ALIVE 0
#define RAY_STATE_TERMINATED 1
#define RAY_STATE_CANCELLED 2
#define RAY_MAX_VOLUMES_ENTERED 4

#define IOR_DEFAULT (1.0f) 

//#define WHITE_FURNACE_TEST
//#define WHITE_FURNACE_TEST_BOUNCE_LIMIT 2