#ifndef REALTIME_COMMON_HLSL_INCL
#define REALTIME_COMMON_HLSL_INCL

#include "../materials/materialsCommon.hlsl"
#include "../common/sceneInstanceData.hlsl"

//keep in sync with RealtimeViewConstants
struct RealtimeViewConstants
{
	float4x4 worldToClip;
	float4 renderResolution;
};

ConstantBuffer<RealtimeViewConstants> g_realtimeView : register(b0, space0);

#endif
