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

	void enteredVolume(float IOR, SpectralSamples absorptionParam)
	{
		numberVolumesEntered = min(numberVolumesEntered + 1, RAY_MAX_VOLUMES_ENTERED);
        ior[numberVolumesEntered - 1] = IOR;
        absorption[numberVolumesEntered - 1] = absorptionParam;
    }
	void exitedVolume()
	{
		numberVolumesEntered = max(0, numberVolumesEntered - 1);
	}

	uint getStateFlags()
	{
		return flags;
	}
	void setStateFlags(uint flagsIn)
	{
		flags = flagsIn;
	}

	void addFlags(uint flags)
	{
		setStateFlags(getStateFlags() | flags);
	}

    SpectralSamples absorption[RAY_MAX_VOLUMES_ENTERED];
	SpectralSamples throughput;
	SpectralSamples totalLight;
    float ior[RAY_MAX_VOLUMES_ENTERED];
	float3 rayOrigin;
	uint rayIndex;
	float3 rayDirection;
	uint pathLength;
	
    float pdfThisRay;
	uint numberVolumesEntered;
	uint rayState;
	uint flags;
};



#endif