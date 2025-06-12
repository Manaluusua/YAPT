#ifndef RAYSTATE_HLSL_INCL
#define RAYSTATE_HLSL_INCL

#define RAYSTATE_FLAGS_SECONDARY_LAMBDAS_TERMINATED (1 << 0)

class RayStateInterface
{
	float getCurrentIOR();
	float getPreviousIOR();

	void enteredVolume(float IOR, SpectralSamples absorption);
	void exitedVolume();

	uint getStateFlags();
	void setStateFlags(uint flags);
};
#endif