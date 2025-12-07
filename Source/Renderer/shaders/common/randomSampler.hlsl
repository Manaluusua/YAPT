#ifndef RANDOM_SAMPLER_HLSL_INCL
#define RANDOM_SAMPLER_HLSL_INCL

#ifndef GET_RANDOM_NUMBER_SEQUENCE_1
#error "GET_RANDOM_NUMBER_SEQUENCE_1 not defined. Includer of randomSampler.hlsl needs to define it"
#endif

#ifndef GET_RANDOM_NUMBER_SEQUENCE_2
#error "GET_RANDOM_NUMBER_SEQUENCE_2 not defined. Includer of randomSampler.hlsl needs to define it"
#endif

#ifndef GET_RANDOM_NUMBER_SEQUENCE_3
#error "GET_RANDOM_NUMBER_SEQUENCE_3 not defined. Includer of randomSampler.hlsl needs to define it"
#endif

#ifndef GET_RANDOM_NUMBER_SEQUENCE_4
#error "GET_RANDOM_NUMBER_SEQUENCE_4 not defined. Includer of randomSampler.hlsl needs to define it"
#endif


uint scramble(uint random, uint scrambleSeed, uint dimension)
{
    uint h = hash(scrambleSeed, dimension);
    return owenScrambleBase2(random, h);
}

float getRandomSampleFloat(uint dimensionSetIndex, uint seed)
{
    uint sobolSeq = GET_RANDOM_NUMBER_SEQUENCE_1(dimensionSetIndex);
    return uintToFloat01(scramble(sobolSeq, seed, dimensionSetIndex));
}

float2 getRandomSampleFloat2(uint dimensionSetIndex, uint seed)
{
    uint2 sobolSeq = GET_RANDOM_NUMBER_SEQUENCE_2(dimensionSetIndex);
    sobolSeq.xy = uint2(scramble(sobolSeq.x, seed, dimensionSetIndex), scramble(sobolSeq.y, seed, dimensionSetIndex + 1));
    return uintToFloat01(sobolSeq);
}

float3 getRandomSampleFloat3(uint dimensionSetIndex, uint seed)
{
    uint3 sobolSeq = GET_RANDOM_NUMBER_SEQUENCE_3(dimensionSetIndex);
    sobolSeq.xyz = uint3(scramble(sobolSeq.x, seed, dimensionSetIndex), scramble(sobolSeq.y, seed, dimensionSetIndex + 1), scramble(sobolSeq.z, seed, dimensionSetIndex + 2));
    return uintToFloat01(sobolSeq);
}

float4 getRandomSampleFloat4(uint dimensionSetIndex, uint seed)
{
    uint4 sobolSeq = GET_RANDOM_NUMBER_SEQUENCE_4(dimensionSetIndex);
    sobolSeq.xyzw = uint4(scramble(sobolSeq.x, seed, dimensionSetIndex), scramble(sobolSeq.y, seed, dimensionSetIndex + 1), scramble(sobolSeq.z, seed, dimensionSetIndex + 2), scramble(sobolSeq.w, seed, dimensionSetIndex + 3));
    return uintToFloat01(sobolSeq);

}

struct RandomSampler
{
    uint2 dimensionOffsetAndSeed;
	
    void init(uint dim, uint pixelIndex)
    {
        dimensionOffsetAndSeed.x = dim + 2; //first 2 dimensions are reserved to texel offset (need to be consistent to allow inferring it in denoise)
        dimensionOffsetAndSeed.y = pixelIndex;
    }
    
    float2 getTexelOffset()
    {
        float2 v = getRandomSampleFloat2(0, dimensionOffsetAndSeed.y);
        return v;
    }
    
    float getRandom1()
    {
        float v = getRandomSampleFloat(dimensionOffsetAndSeed.x, dimensionOffsetAndSeed.y);
        ++dimensionOffsetAndSeed.x;
        return v;
    }
	
    float2 getRandom2()
    {
        float2 v = getRandomSampleFloat2(dimensionOffsetAndSeed.x, dimensionOffsetAndSeed.y);
        dimensionOffsetAndSeed.x += 2;
        return v;
    }
    float3 getRandom3()
    {
        float3 v = getRandomSampleFloat3(dimensionOffsetAndSeed.x, dimensionOffsetAndSeed.y);
        dimensionOffsetAndSeed.x += 3;
        return v;
    }
	
    float4 getRandom4()
    {

        float4 v = getRandomSampleFloat4(dimensionOffsetAndSeed.x, dimensionOffsetAndSeed.y);
        dimensionOffsetAndSeed.x += 4;
        return v;
    }
	
    
};


#endif