#ifndef COMMON_HLSL_INCL
#define COMMON_HLSL_INCL

#define SAFE_DIVISOR(x) (((x) == 0.f) ? (1.f) : (x))

bool isNan(float val)
{
	return isnan(val) || val != val;
}

bool isNan(float2 val)
{
	return isNan(val.x) || isNan(val.y);
}

bool isNan(float3 val)
{
	return isNan(val.x) || isNan(val.y) || isNan(val.z);
}

bool isNan(float4 val)
{
	return isNan(val.xy) || isNan(val.zw);
}

bool isZero(float2 vec)
{
	return dot(vec, vec) == 0.f;
}

bool isZero(float3 vec)
{
	return dot(vec, vec) == 0.f;
}

bool isZero(float4 vec)
{
	return dot(vec, vec) == 0.f;
}

#endif