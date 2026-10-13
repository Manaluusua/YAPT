#ifndef SUBSURFACE_HLSL_INCL
#define SUBSURFACE_HLSL_INCL
/*
Subsurface scattering (Christensen-Burley diffusion profile) as a separable BSSRDF: S(po, wo, pi, wi) = entry(po, wo) * Rd(|po - pi|) * Sw(pi, wi)
A path that takes the subsurface event enters at po and continues from the exit point pi; the edge po -> pi is not a ray and can't be used for connections.
*/

struct SurfacePoint
{
	float3 positionWS;
	uint instanceIndex;
	float3 geometryNormalWS; //oriented to the outside, ie. the side the path enters from or leaves to
	uint primitiveIndex;
	float2 barycentrics;
	bool frontFace;
};

struct SubsurfaceExitSample
{
	SurfacePoint exitPoint;
	SpectralSamples weight; //Rd(|po - pi|) / pdfPosition
	float pdfPosition; //area density of the exit point
};

float getSubsurfaceLayerWeight(in SurfaceDefinition surfaceDef)
{
	return (1.f - surfaceDef.metalness) * (1.f - surfaceDef.transparency) * surfaceDef.subsurfaceAmount;
}


float getEnergyLeftForBaseLayer(in SurfaceDefinition surfaceDef, in PrecalculatedSurfaceData preCalcData)
{
	float energyLeft = 1.f;
	float fromIOR = preCalcData.fromIOR;
	float toIOR = preCalcData.toIOR;

	if (surfaceDef.clearCoatAmount > 0)
	{
		float2 a2CC = calculateRoughnessParams(surfaceDef.clearCoatRoughness, 0.f);
		ReflectionDielectric coating = ReflectionDielectric::init(a2CC, surfaceDef.clearCoatRoughness, surfaceDef.clearCoatIOR / fromIOR);
		energyLeft *= coating.getEnergyLeftAfterLayer(preCalcData.woCoating, preCalcData.woCoating, surfaceDef.clearCoatAmount);
	}

	if (surfaceDef.sheenAmount > 0)
	{
		ReflectionSheen sheen = ReflectionSheen::init(surfaceDef.sheenColor, surfaceDef.sheenRoughness);
		energyLeft *= sheen.getEnergyLeftAfterLayer(preCalcData.woBase, preCalcData.woBase, surfaceDef.sheenAmount);
	}

	if (surfaceDef.metalness > 0)
	{
		energyLeft *= 1.f - surfaceDef.metalness;
	}

	float specAmount = (1.f - surfaceDef.metalness) * surfaceDef.specularAmount;
	if (specAmount > 0)
	{
		if (hasDispersion(surfaceDef.flags))
		{
			if (preCalcData.exiting)
			{
				fromIOR = getRefractiveIndexForWavelength(surfaceDef.cauchysCoeffs, getHeroSpectralLambda());
			}
			else
			{
				toIOR = getRefractiveIndexForWavelength(surfaceDef.cauchysCoeffs, getHeroSpectralLambda());
			}
		}
		SpecularLayer specLayer = SpecularLayer::init(preCalcData.a2, surfaceDef.roughness, toIOR / fromIOR, true, surfaceDef.transparency > 0);
		energyLeft *= specLayer.getEnergyLeftAfterLayer(preCalcData.woBase, preCalcData.woBase, specAmount);
	}

	return energyLeft;
}
float getSubsurfaceEntryFactor(in SurfaceDefinition surfaceDef, in PrecalculatedSurfaceData preCalcData)
{
	float specOcclusion;
	float diffOcclusion;
	unpackOcclusion(surfaceDef.occlusionSpecDiffPacked, diffOcclusion, specOcclusion);

	return getSubsurfaceLayerWeight(surfaceDef) * getEnergyLeftForBaseLayer(surfaceDef, preCalcData) * diffOcclusion;
}


bool sampleSubsurfaceExit(in SurfaceDefinition surfaceDef, in SurfacePoint entryPoint, in float3 rand, out SubsurfaceExitSample exitOut)
{
	exitOut.exitPoint = entryPoint;
	exitOut.weight = surfaceDef.albedo;
	exitOut.pdfPosition = 1.f;
	return true;
}

float pdfSubsurfaceExit(in SurfaceDefinition surfaceDef, in float3 fromPosWS, in float3 fromNormalWS, in float3 toPosWS, in float3 toNormalWS)
{
	return 1.f;
}


float3 sampleSubsurfaceExitLobe(in SurfaceDefinition surfaceDef, in PrecalculatedSurfaceData preCalcData, in float3 woOS, in float2 rand)
{
	float3 wi = sampleCosineWeightedHemisphere(rand);
	wi = mul(wi, preCalcData.toBaseLayerTangentSpace(surfaceDef.tangent));

	//the shading normal can disagree with the geometry, don't leave to the inside
	if (dot(wi, surfaceDef.geometryNormal) * dot(woOS, surfaceDef.geometryNormal) <= 0)
	{
		return 0.f;
	}
	return wi;
}

void evaluateSubsurfaceExitLobe(in SurfaceDefinition surfaceDef, in PrecalculatedSurfaceData preCalcData, in float3 woOS, in float3 wiOS, in uint evaluateFlags, out SpectralSamples weightOut, out float pdfOut)
{
	weightOut.set(0);
	pdfOut = 0.f;

	float dotWoGeometry = dot(woOS, surfaceDef.geometryNormal);
	float dotWiGeometry = dot(wiOS, surfaceDef.geometryNormal);
	if (dotWoGeometry * dotWiGeometry <= 0)
	{
		return;
	}

	float3 wiBase = normalize(mul(preCalcData.toBaseLayerTangentSpace(surfaceDef.tangent), wiOS));
	if (wiBase.y <= 0)
	{
		return;
	}

	pdfOut = pdfCosineWeightedHemisphere(wiBase.y);

	if ((evaluateFlags & EVALUATE_FLAGS_PDF_ONLY) == 0)
	{
		//Sw has no dependency on the arriving direction, so on a light path the shading cosine is replaced by the geometric one of the leaving direction
		bool lightPath = (evaluateFlags & EVALUATE_FLAGS_LIGHT_PATH) != 0;
		float cosine = lightPath ? abs(dotWiGeometry) : wiBase.y;
		weightOut.set(INVPI * cosine);
	}
}

#endif
