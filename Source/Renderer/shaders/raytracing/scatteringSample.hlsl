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

struct ScatteringEvaluation
{
	SpectralSamples weight;
	float pdfForward;
	float pdfBackward;
	bool dispersed;
};


ScatteringEvent sampleScatteringEvent(in SurfaceSamplingContext surface, in SurfacePoint vertexPoint, in float3 woOS, in bool atSubsurfaceExit, in float4 rand)
{
	ScatteringEvent ev = (ScatteringEvent)0;
	ev.type = SCATTERING_EVENT_NONE;

	if (atSubsurfaceExit)
	{
		ev.wiOS = sampleSubsurfaceExitLobe(surface.surfaceDef, surface.precalculatedSurfaceData, woOS, rand.zw);
		ev.isDiffuseBounce = true;
		ev.isDeltaDistribution = false;
		if (!isZero(ev.wiOS))
		{
			ev.type = SCATTERING_EVENT_SUBSURFACE_EXIT_LOBE;
		}
		return ev;
	}

	uint layer = selectMaterialLayer(rand.x, surface.samplingProbabilities);
	if (layer == LAYERIND_SUBSURFACE)
	{
		ev.isDiffuseBounce = true;
		ev.isDeltaDistribution = false;
		if (sampleSubsurfaceExit(surface.surfaceDef, vertexPoint, rand.yzw, ev.exit))
		{
			ev.type = SCATTERING_EVENT_SUBSURFACE_ENTRY;
		}
		return ev;
	}

	ev.wiOS = getSampleDirectionOSForLayer(surface, layer, rand.y, rand.zw, ev.isDiffuseBounce, ev.isDeltaDistribution);
	if (!isZero(ev.wiOS))
	{
		ev.type = SCATTERING_EVENT_LOCAL;
	}
	return ev;
}

//pdfForward is an area density over the subsurface edge, pdfBackward the exit lobe density of wo (the reverse path would leave here)
ScatteringEvaluation evaluateSubsurfaceEntry(in SurfaceSamplingContext surface, in float3 woOS, in SubsurfaceExitSample exit)
{
	ScatteringEvaluation eval;
	eval.weight = exit.weight * (getSubsurfaceEntryFactor(surface.surfaceDef, surface.precalculatedSurfaceData) * exit.pdfPosition);
	eval.pdfForward = surface.samplingProbabilities[LAYERIND_SUBSURFACE] * exit.pdfPosition;
	eval.dispersed = false;

	SpectralSamples weightDummy;
	evaluateSubsurfaceExitLobe(surface.surfaceDef, surface.precalculatedSurfaceData, woOS, woOS, EVALUATE_FLAGS_PDF_ONLY, weightDummy, eval.pdfBackward);
	return eval;
}

#endif
