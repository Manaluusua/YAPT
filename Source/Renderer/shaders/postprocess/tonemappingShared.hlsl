#ifndef INCL_TONEMAPPING_SHARED_HLSL
#define INCL_TONEMAPPING_SHARED_HLSL

#include "../globalDefinitions.hlsl"

#define HISTOGRAM_BUCKETS_COUNT 128
#define GATHER_LUMINANCE_TG_SIZE 16

//ISO/Calibration
#define ISOCALIB (100.f / 12.5) 
#define ISOCALIBINV (1.f / ISOCALIB)

float fromLuminanceToEV(float lum)
{
	return log2(lum * ISOCALIB);
}

float fromEVToLuminance(float ev)
{
	return exp2(ev) * ISOCALIBINV;
}


struct HistogramConstants
{
	uint2 resolution;
	float minEV;
	float pad0;
};



struct PrepareExposureInfoData
{
	float4 unused;
};


struct TonemapConstants
{
	HistogramConstants histogramConstants;
	PrepareExposureInfoData prepareTonemapDataConstants;
	float4 toeMidShoulderInit;
	float4 exposureEyeAdaptTime;
	uint4 operatorEnabled; //x: apply the tonemapping operator, exposure is applied regardless
};



struct LuminanceHistogramAnalysisResults
{
	float averageLuminance;
	float minLuminance;
	float maxLuminance;
};

struct ExposureInfo
{
	float cameraExposure;
};


#endif