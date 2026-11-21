struct Input
{
	float4 position : SV_Position;
	nointerpolation uint instanceIndex : TEXCOORD0;
	nointerpolation uint primitiveIndex : TEXCOORD1;
	float2 uv : TEXCOORD2;
};

uint4 main(Input input) : SV_Target0
{
	return uint4(input.instanceIndex + 1, input.primitiveIndex, asuint(input.uv.x), asuint(input.uv.y));
}
