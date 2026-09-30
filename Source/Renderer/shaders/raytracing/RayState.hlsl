#ifndef RAYSTATE_HLSL_INCL
#define RAYSTATE_HLSL_INCL

#define RAYSTATE_FLAGS_SECONDARY_LAMBDAS_TERMINATED (1 << 0)
#define RAYSTATE_FLAGS_REGULARIZE_PATH (1 << 1)
#define RAYSTATE_FLAGS_SAMPLED_FROM_DELTA_DISTRIBUTION (1 << 2)

#define VOLUME_MATERIAL_NONE 0xFFFFFFFF


bool shouldStartRegularizing(uint vertexIndex, uint regularizeAfterVertices)
{
	return vertexIndex >= regularizeAfterVertices;
}

class RayStateInterface
{
	float getCurrentIOR();
	float getPreviousIOR();

	void enteredVolume(float IOR, SpectralSamples absorption, uint materialIndex);
	void exitedVolume(uint materialIndex);

	uint getStateFlags();
	void setStateFlags(uint flags);
};
#endif