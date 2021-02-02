//ACES 
//taken from https://github.com/TheRealMJP/BakingLab/blob/master/BakingLab/ACES.hlsl
// sRGB => XYZ => D65_2_D60 => AP1 => RRT_SAT
static const float3x3 ACESInputMat =
{
    {0.59719, 0.35458, 0.04823},
    {0.07600, 0.90834, 0.01566},
    {0.02840, 0.13383, 0.83777}
};

// ODT_SAT => XYZ => D60_2_D65 => sRGB
static const float3x3 ACESOutputMat =
{
    { 1.60475, -0.53108, -0.07367},
    {-0.10208,  1.10813, -0.00605},
    {-0.00327, -0.07276,  1.07602}
};

float3 RRTAndODTFit(float3 v)
{
    float3 a = v * (v + 0.0245786f) - 0.000090537f;
    float3 b = v * (0.983729f * v + 0.4329510f) + 0.238081f;
    return a / b;
}

float3 ACESFitted(float3 color)
{
    color = mul(ACESInputMat, color);

    // Apply RRT and ODT
    color = RRTAndODTFit(color);

    color = mul(ACESOutputMat, color);

    // Clamp to [0, 1]
    color = saturate(color);

    return color;
}

//Hable (uncharted2)
float3 uncharted2Tonemap(float3 color, float toe, float mid, float shoulder)
{
	const float shoulderStr = shoulder;//0.15;
	const float linearStr = mid; //0.50;
	const float linearAngle = 0.10;
	const float toeStr = toe; //0.20;
	const float toeNum = 0.02;
	const float toeDenom = 0.30;
	
	return ((color*(shoulderStr*color+linearAngle*linearStr)+toeStr*toeNum)/(color*(shoulderStr*color+linearStr)+toeStr*toeDenom))-toeNum/toeDenom;
}

float3 filmicTonemap(float3 color, float toe, float mid, float shoulder)
{
	const float whiteScale = 11.2;
	float3 currCol = uncharted2Tonemap(color, toe, mid, shoulder)/uncharted2Tonemap(whiteScale, toe, mid, shoulder);
	return currCol;
}

float getPerceivedLuminance(float3 col)
{
	return dot(col, float3(0.2126, 0.7152, 0.0722));
}
