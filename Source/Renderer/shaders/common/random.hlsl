#ifndef RANDOM_HLSL_INCL
#define RANDOM_HLSL_INCL

#define FLOAT_ONE_MINUS_EPSILON (0x1.fffffep-1)
#define FLOAT_UINT_MAX_INVERSE (0x1p-32f)

float vanDerCorputSequence(uint index)
{
	const float invMax = 1.0 / 0xFFFFFFFF;
	return float(reversebits(index)) * invMax ; 
}

float halton(int b, int i)
{
    float r = 0.0;
    float f = 1.0;
    while (i > 0) {
        f = f / float(b);
        r = r + f * float(i % b);
        i = int(floor(float(i) / float(b)));
    }
    return r;
}

float2 weyl(int i)
{
    return frac(float2(i*float2(12664745, 9560333))/exp2(24.0)); // integer mul to avoid round-off
}

float2 hammersley(int i, int n)
{
    return float2(vanDerCorputSequence(i), float(i)/float(n));
}

uint murmurhash(uint x)
{
    x ^= x >> 16;
    x *= 0x85ebca6b;
    x ^= x >> 13;
    x *= 0xc2b2ae35;
    x ^= x >> 16;
    return x;
}

uint hash(uint a)
{
    return murmurhash(a);
}

uint hash(uint a, uint b)
{
    uint key = ((a + b) * (a + b + 1)) / (2 + b);
    return murmurhash(key);
}

uint owenScrambleBase2(uint v, uint seed)
{
    v = reversebits(v);
    v ^= v * 0x3d20adea;
    v += seed;
    v *= (seed >> 16) | 1;
    v ^= v * 0x05526c56;
    v ^= v * 0x53a22864;
    return reversebits(v);
}


//[0, 0xFFFFFFFF] -> [0, 1)
float uintToFloat01(uint randValue)
{
    return min(randValue * FLOAT_UINT_MAX_INVERSE, FLOAT_ONE_MINUS_EPSILON);
}

float2 uintToFloat01(uint2 randValue)
{
    return float2(uintToFloat01(randValue.x), uintToFloat01(randValue.y));

}

float3 uintToFloat01(uint3 randValue)
{
    return float3(uintToFloat01(randValue.x), uintToFloat01(randValue.y), uintToFloat01(randValue.z));

}

float4 uintToFloat01(uint4 randValue)
{
    return float4(uintToFloat01(randValue.x), uintToFloat01(randValue.y), uintToFloat01(randValue.z), uintToFloat01(randValue.w));

}

#endif