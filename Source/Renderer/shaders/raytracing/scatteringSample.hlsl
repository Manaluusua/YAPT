#ifndef SCATTERINGSAMPLE_HLSL_INCL
#define SCATTERINGSAMPLE_HLSL_INCL


#include "subsurface.hlsl"

#define SCATTERING_EVENT_NONE 0
#define SCATTERING_EVENT_LOCAL 1
#define SCATTERING_EVENT_SUBSURFACE_ENTRY 2
#define SCATTERING_EVENT_SUBSURFACE_EXIT_LOBE 3

struct ScatteringEvent
{
	SubsurfaceExitSample exit; //SCATTERING_EVENT_SUBSURFACE_ENTRY only
	float3 wiOS; //SCATTERING_EVENT_LOCAL and SCATTERING_EVENT_SUBSURFACE_EXIT_LOBE only
	uint type;
	bool isDiffuseBounce;
	bool isDeltaDistribution;
};


ScatteringEvent sampleScatteringEvent(in SurfaceDefinition surfaceDef, in PrecalculatedSurfaceData preCalcData, in float samplingProbabilities[LAYER_COUNT], in bool atSubsurfaceExit, in float3 woOS, in float4 rand, in SurfacePoint vertexPoint)
{
	ScatteringEvent ev = (ScatteringEvent)0;
	ev.type = SCATTERING_EVENT_NONE;

	if (atSubsurfaceExit)
	{
		ev.wiOS = sampleSubsurfaceExitLobe(surfaceDef, preCalcData, woOS, rand.zw);
		ev.isDiffuseBounce = true;
		ev.isDeltaDistribution = false;
		if (!isZero(ev.wiOS))
		{
			ev.type = SCATTERING_EVENT_SUBSURFACE_EXIT_LOBE;
		}
		return ev;
	}

	uint layer = selectMaterialLayer(rand.x, samplingProbabilities);
	if (layer == LAYERIND_SUBSURFACE)
	{
		ev.isDiffuseBounce = true;
		ev.isDeltaDistribution = false;
		if (sampleSubsurfaceExit(surfaceDef, vertexPoint, rand.yzw, ev.exit))
		{
			ev.type = SCATTERING_EVENT_SUBSURFACE_ENTRY;
		}
		return ev;
	}

	ev.wiOS = getSampleDirectionOSForLayer(surfaceDef, preCalcData, layer, rand.y, rand.zw, ev.isDiffuseBounce, ev.isDeltaDistribution);
	if (!isZero(ev.wiOS))
	{
		ev.type = SCATTERING_EVENT_LOCAL;
	}
	return ev;
}

SpectralSamples getSubsurfaceEntryWeight(in SurfaceDefinition surfaceDef, in PrecalculatedSurfaceData preCalcData, in float samplingProbabilities[LAYER_COUNT], in SubsurfaceExitSample exit)
{
	float entryFactor = getSubsurfaceEntryFactor(surfaceDef, preCalcData);
	return exit.weight * safeDiv(entryFactor, samplingProbabilities[LAYERIND_SUBSURFACE]);
}

#endif
