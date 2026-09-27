#ifndef PAYLOAD_HLSL_INCL
#define PAYLOAD_HLSL_INCL

//the resources needed are defined in raytraceCommonResources.hlsl
#include "rayState.hlsl"

struct Payload //: RayStateInterface
{
	uint getRayIndex()
	{
		return rayIndex;
	}
	uint getPathLength()
	{
		return pathLength;
	}
	
    SpectralSamples getAbsorption()
    {
        if (numberVolumesEntered != 0)
        {
            return absorption[numberVolumesEntered - 1];
        }
        else
        {
            SpectralSamples abs;
            abs.set(0);
            return abs;

        }
    }

	float getCurrentIOR()
	{
		float currentIOR = IOR_DEFAULT; //air if not entered volume
		if (numberVolumesEntered != 0)
		{
            currentIOR = ior[numberVolumesEntered - 1];
        }
		return currentIOR;
	}
	float getPreviousIOR()
	{
		float beforeCurrentIOR = IOR_DEFAULT;
		if (numberVolumesEntered > 1)
		{
            beforeCurrentIOR = ior[numberVolumesEntered - 2];
        }
		return beforeCurrentIOR;
	}


    int findVolume(uint materialIndex)
    {
        for (int i = int(numberVolumesEntered) - 1; i >= 0; --i)
        {
            if (volumeMaterial[i] == materialIndex)
            {
                return i;
            }
        }
        return -1;
    }

    void enteredVolume(float IOR, SpectralSamples absorptionParam, uint materialIndex)
    {
        numberVolumesEntered = min(numberVolumesEntered + 1, RAY_MAX_VOLUMES_ENTERED);
        ior[numberVolumesEntered - 1] = IOR;
        absorption[numberVolumesEntered - 1] = absorptionParam;
        volumeMaterial[numberVolumesEntered - 1] = materialIndex;
    }


    void exitedVolume(uint materialIndex)
    {
        if (numberVolumesEntered == 0)
        {
            return;
        }
        int index = findVolume(materialIndex);
        if (index < 0)
        {
            index = int(numberVolumesEntered) - 1;
        }
        for (uint i = uint(index); i + 1 < numberVolumesEntered; ++i)
        {
            ior[i] = ior[i + 1];
            absorption[i] = absorption[i + 1];
            volumeMaterial[i] = volumeMaterial[i + 1];
        }
        --numberVolumesEntered;
    }

    bool isFalseIntersection(uint materialIndex, bool hitFrontFace)
    {
        if (hitFrontFace || numberVolumesEntered < 2)
        {
            return false;
        }
        int index = findVolume(materialIndex);
        return index >= 0 && index != int(numberVolumesEntered) - 1;
    }

	uint getStateFlags()
	{
		return flags;
	}
	void setStateFlags(uint flagsIn)
	{
		flags = flagsIn;
	}

    void addFlags(uint flagsIn)
	{
        setStateFlags(getStateFlags() | flagsIn);
    }
	
    void removeFlags(uint flagsIn)
    {
        setStateFlags(getStateFlags() & ~flagsIn);
    }
	
    bool hasFlags(uint flagsIn)
    {
        return (getStateFlags() & flagsIn) == flagsIn;
    }
	
    uint getNumberOfVolumesEntered()
    {
        return numberVolumesEntered;
    }
    SpectralSamples absorption[RAY_MAX_VOLUMES_ENTERED];
	SpectralSamples throughput;
	SpectralSamples totalLight;
    float4 ior;
    uint4 volumeMaterial;
	float3 segmentOrigin; //last real path vertex, the ray origin can move past false intersections
	uint falseIntersections;
	float3 rayOrigin;
	uint rayIndex;
	float3 rayDirection;
	uint pathLength;
	
    uint2 randomDimensionOffsetAndScramble;
    float sampledWavelength;
    float pdfThisRay;
	uint numberVolumesEntered;
	uint rayState;
	uint flags;
};



#endif