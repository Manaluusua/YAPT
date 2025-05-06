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

	float getCurrentIOR()
	{
		float currentIOR = IOR_DEFAULT; //air if not entered volume
		if (numberVolumesEntered != 0)
		{
			currentIOR = volumesEntered[numberVolumesEntered - 1];
		}
		return currentIOR;
	}
	float getPreviousIOR()
	{
		float beforeCurrentIOR = IOR_DEFAULT;
		if (numberVolumesEntered > 1)
		{
			beforeCurrentIOR = volumesEntered[numberVolumesEntered - 2];
		}
		return beforeCurrentIOR;
	}

	void enteredVolume(float IOR, SpectralSamples absorptionParam)
	{
		numberVolumesEntered = min(numberVolumesEntered + 1, RAY_MAX_VOLUMES_ENTERED);
		volumesEntered[numberVolumesEntered - 1] = IOR;
		absorption = absorptionParam;
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


	SpectralSamples throughput;
	SpectralSamples absorption;
	SpectralSamples totalLight;
	float volumesEntered[RAY_MAX_VOLUMES_ENTERED];
	float3 rayOrigin;
	uint rayIndex;
	float3 rayDirection;
	uint pathLength;

	uint numberVolumesEntered;
	uint rayState;
	uint flags;
};



#endif