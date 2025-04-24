#ifndef PAYLOAD_HLSL_INCL
#define PAYLOAD_HLSL_INCL

//the resources needed are defined in raytraceCommonResources.hlsl

#define PAYLOAD_FLAGS_SECONDARY_LAMBDAS_TERMINATED (1 << 0)

struct Payload
{
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

float payloadGetCurrentIOR(in Payload payload)
{
	float currentIOR = IOR_DEFAULT; //air if not entered volume
	if (payload.numberVolumesEntered != 0)
	{
		currentIOR = payload.volumesEntered[payload.numberVolumesEntered - 1];
	}
	return currentIOR;
}

float payloadGetBeforeCurrentIOR(in Payload payload)
{
	float beforeCurrentIOR = IOR_DEFAULT;
	if (payload.numberVolumesEntered > 1)
	{
		beforeCurrentIOR = payload.volumesEntered[payload.numberVolumesEntered - 2];
	}
	return beforeCurrentIOR;
}

void payloadRayEnteredVolume(inout Payload payload, in float IOROfEnteredVolume)
{
	payload.numberVolumesEntered = min(payload.numberVolumesEntered + 1, RAY_MAX_VOLUMES_ENTERED);
	payload.volumesEntered[payload.numberVolumesEntered - 1] = IOROfEnteredVolume;
}

void payloadRayExitedVolume(inout Payload payload)
{
	payload.numberVolumesEntered = max(0, payload.numberVolumesEntered - 1);
}

#endif