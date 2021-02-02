#include "tonemappingShared.hlsl"
#include "tonemapUtils.hlsl"

Texture2D colorTex : register(t0, space0);
SamplerState colorSampler : register(s1, space0);
ConstantBuffer<TonemapConstants> g_tonemapConstants : register(b2, space0);
StructuredBuffer<ExposureInfo> g_exposureInfo : register(t3, space0);

struct Input
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD0;
};

float4 tonemap(Input input) : SV_TARGET
{
	ExposureInfo info = g_exposureInfo[0];
	float3 toeMidShoulder = g_tonemapConstants.toeMidShoulderInit.xyz;

    float4 color = colorTex.Sample(colorSampler, input.uv);
	color = info.cameraExposure * color;
	
#ifdef TONEMAP_ACES
	color.xyz = ACESFitted(color.xyz);
#elif TONEMAP_UNCHARTED
	color.xyz = filmicTonemap(color.xyz, toeMidShoulder.x, toeMidShoulder.y, toeMidShoulder.z);
#endif

    return color;
}