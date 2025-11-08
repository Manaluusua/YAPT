#ifndef SPECTRAL_CONSTANTS_H
#define SPECTRAL_CONSTANTS_H

#define SPECTRAL_SAMPLES_COUNT 4

#define CIE_LUT_ORIGINAL_RESOLUTION 471

//Values for full range of 360-830 from CIE database
/*
#define CIE_LUT_ARRAY_OFFSET 0
#define CIE_LUT_LAMBDA_MIN 360
#define CIE_LUT_LAMBDA_MAX 830
#define CIE_LUT_RESOLUTION 471
#define CIE_Y_SUM 106.8569171
#define CIE_Y_SUM_INV 0.00935830854
#define CIE_D65_SUM 39187.6996
#define CIE_D65_SUM_INV 0.00002551
*/

//Values for narrower range as the full range is mostly zero on the edges due to floating point precision
#define CIE_LUT_ARRAY_OFFSET 20
#define CIE_LUT_LAMBDA_MIN 380
#define CIE_LUT_LAMBDA_MAX 700
#define CIE_LUT_RESOLUTION 321
#define CIE_Y_SUM 106.7989583
#define CIE_Y_SUM_INV 0.00936338721
#define CIE_D65_SUM 29988.647
#define CIE_D65_SUM_INV 0.00003334595

#define SRGB_TO_SPD_RES 64
#define REC2020_TO_SPD_RES 64
#endif