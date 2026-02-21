#ifndef RAYSTATE_HLSL_INCL
#define RAYSTATE_HLSL_INCL

#define RAYSTATE_FLAGS_SECONDARY_LAMBDAS_TERMINATED (1 << 0)
#define RAYSTATE_FLAGS_HAS_DIFFUSE_BOUNCE (1 << 1)
#define RAYSTATE_FLAGS_SAMPLED_FROM_DELTA_DISTRIBUTION (1 << 2)

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