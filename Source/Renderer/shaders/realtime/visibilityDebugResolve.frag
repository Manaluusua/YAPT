#include "../common/random.hlsl"

Texture2D<uint4> g_visibility : register(t0, space0);

struct Input
{
	float4 pos : SV_POSITION;
	float2 uv : TEXCOORD0;
};

float3 hashToColor(uint h)
{
	return float3(h & 0xFF, (h >> 8) & 0xFF, (h >> 16) & 0xFF) / 255.f;
}

float4 main(Input input) : SV_TARGET
{
	uint4 visibility = g_visibility.Load(int3(input.pos.xy, 0));
	if (visibility.x == 0)
	{
		return float4(0.05f, 0.05f, 0.05f, 1.f);
	}

	uint instanceIndex = visibility.x - 1;
	uint primitiveId = visibility.y;
	float2 uv = asfloat(visibility.zw);

	float3 instanceColor = hashToColor(hash(instanceIndex + 1)) * 0.8f + 0.2f;
	float primitiveShade = 0.85f + 0.15f * ((hash(primitiveId, instanceIndex) & 0xFF) / 255.f);
	float2 uvCell = floor(frac(uv) * 8.f);
	float checker = (uint(uvCell.x + uvCell.y) & 1) != 0 ? 1.f : 0.75f;

	return float4(instanceColor * primitiveShade * checker, 1.f);
}
