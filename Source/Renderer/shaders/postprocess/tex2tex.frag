Texture2D colorTex : register(t0, space0);
SamplerState colorSampler : register(s1, space0);

#ifdef MULTIPLY_WITH_UBO_ALPHA
struct Tex2TexParams
{
	float alpha;
};
ConstantBuffer<Tex2TexParams> g_tex2TexParams : register(b2, space0);
#endif

struct Input
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD0;
};

float4 tex2tex(Input input) : SV_TARGET
{
    float4 color = colorTex.Sample(colorSampler, input.uv);
#ifdef MULTIPLY_WITH_UBO_ALPHA
	color *= g_tex2TexParams.alpha;
#endif
    return color;
}