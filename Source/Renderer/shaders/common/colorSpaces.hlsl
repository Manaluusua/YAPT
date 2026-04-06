#ifndef COLORSPACES_HLSL
#define COLORSPACES_HLSL

static const float3x3 c_rec2020XYZToRGB =
{
	1.7166512, -0.3556708, -0.2533663,
	-0.6666844, 1.6164812, 0.0157685,
	0.0176399, -0.0427706, 0.9421031
};

static const float3x3 c_rec2020RGBToXYZ =
{
	0.6369580,  0.1446169,  0.1688810,
	0.2627002,  0.6779981,  0.0593017,
	0.0000000,  0.0280727,  1.0609851
};

static const float3x3 c_srgbXYZToRGB =
{
	3.2406, -1.5372, -0.4986,
	-0.9689, 1.8758, 0.0415,
	0.0557, -0.2040, 1.0570,
};

static const float3x3 c_srgbRGBToXYZ =
{
	0.4124, 0.3576, 0.1805,
	0.2126, 0.7152, 0.0722,
	0.0193, 0.1192, 0.9505
};

float3 fromRGBToYCoCg(float3 rgb)
{
    float orange = rgb.r - rgb.b;
    float tmp = rgb.b + orange * 0.5f;
    float green = rgb.g - tmp;
    float y = tmp + green * 0.5f;
    return float3(y, orange, green);
}

float3 fromYCoCgtoRGB(float3 yog)
{
    float tmp = yog.x - yog.z * 0.5f;
    float g = yog.z + tmp;
    float b = tmp - yog.y * 0.5f;
    float r = b + yog.y;
	
    return float3(r, g, b);
}

#endif