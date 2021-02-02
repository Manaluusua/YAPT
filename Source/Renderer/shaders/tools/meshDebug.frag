struct Input
{
    float4 pos : SV_POSITION;
    float3 color : COLOR0;
};

float4 main(Input input) : SV_TARGET
{
    
    return float4(input.color, 1.f);
}