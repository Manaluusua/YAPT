#ifndef MATERIALSAMPLE_HLSL_INCL
#define MATERIALSAMPLE_HLSL_INCL

#include "materialLayers.hlsl"

#define LAYERIND_COATING_GGX 0
#define LAYERIND_COATING_SHEEN 1
#define LAYERIND_SPEC_CONDUCTOR 2
#define LAYERIND_SPEC_DIELECTRIC 3
#define LAYERIND_DIFFUSE_REFL 4
#define LAYERIND_TRANSMITTED 5

#define LAYER_COUNT (LAYERIND_TRANSMITTED + 1)

#define MAT_SAMPLING_ADD_LAYER(layer, probability) samplingProbabilities[layer] = (probability); sampleSum += (probability);

struct PrecalculatedSurfaceData
{
	float2 a2;
	float3 woBase;
	float3 woCoating;
	float fromIOR;
	float toIOR;
	bool exiting;
};

void calculateNormalizedMaterialLayerSamplingProbabilities(in SurfaceDefinition surfaceDef, 
	in float3 woBase,in float3 woCoating, in float fromIOR, in float toIOR, in float2 a2, float2 rand,
	out float samplingProbabilities[LAYER_COUNT])
{
	//decide here if we will sample reflections or transmission
	float3 wm = sampleWMGGX(woBase, a2.x, a2.y, rand.x, rand.y);
	float F = fresnelDielectricDielectric2(toIOR/fromIOR, dot(woBase, wm));
	
	bool comingFromInside = woBase.y < 0;
	float fromInsideMultiplier = comingFromInside ? 0.f : 1.f;
	float refrProb = (1.f - F) * surfaceDef.transparency;
	float reflProb = 1.f - refrProb;
	
	//explicit TIR handling (enable if Fresnel doesn't take TIR into account)
	/*float3 wi = refract(-woBase, wm, fromIOR/toIOR);
	if(isZero(wi))
	{
		reflProb = 1.f;
		refrProb = 0.f;
	}*/

	float sampleSum = 0.f;
	MAT_SAMPLING_ADD_LAYER(LAYERIND_COATING_GGX, surfaceDef.clearCoatAmount * reflProb * fromInsideMultiplier);
	MAT_SAMPLING_ADD_LAYER(LAYERIND_COATING_SHEEN, surfaceDef.sheenAmount * reflProb * fromInsideMultiplier);
	MAT_SAMPLING_ADD_LAYER(LAYERIND_SPEC_CONDUCTOR, surfaceDef.metalness * reflProb * fromInsideMultiplier);
	MAT_SAMPLING_ADD_LAYER(LAYERIND_SPEC_DIELECTRIC, ((1.f - surfaceDef.metalness) * surfaceDef.specularAmount) * reflProb);
	MAT_SAMPLING_ADD_LAYER(LAYERIND_DIFFUSE_REFL, (1.f - surfaceDef.transparency) * reflProb * fromInsideMultiplier);
	
	MAT_SAMPLING_ADD_LAYER(LAYERIND_TRANSMITTED, refrProb);
	
	sampleSum = max(0.000001f, sampleSum);
	
	for(uint i = 0; i < LAYER_COUNT; ++i)
	{
		samplingProbabilities[i] /= sampleSum;
	}

}

void calculateNormalizedMaterialLayerSamplingProbabilities(in SurfaceDefinition surfaceDef, in PrecalculatedSurfaceData preCalcData, float2 rand, out float samplingProbabilities[LAYER_COUNT])
{
	calculateNormalizedMaterialLayerSamplingProbabilities(surfaceDef, preCalcData.woBase, preCalcData.woCoating, preCalcData.fromIOR, preCalcData.toIOR, preCalcData.a2, rand, samplingProbabilities);
}

template<typename RayState>
float3 getSampleDirection(in SurfaceDefinition surfaceDef, in RayState rayState, in float3 woBase, in float3 woCoating, in float2 a2, in float randMaterialType, in float2 randSampleBrdf, in float samplingProbabilities[LAYER_COUNT])
{	
	float materialTypeRand = max(0, randMaterialType + 0.00001f);
	
	float3 wi = 0;
	float allowTransmitted = surfaceDef.transparency > 0;
	
	uint sampleLayer;
	for(sampleLayer = 0; sampleLayer < LAYER_COUNT; ++sampleLayer)
	{
		if(materialTypeRand < samplingProbabilities[sampleLayer] && samplingProbabilities[sampleLayer] > 0)
		{
			break;
		}
		materialTypeRand -= samplingProbabilities[sampleLayer];
	}
	
	if(sampleLayer == LAYERIND_COATING_GGX)
	{
		if(woCoating.y > 0)
		{
			float2 a2CC = calculateRoughnessParams(surfaceDef.clearCoatRoughness, 0.f);
			wi = sampleGGXReflectionDielectric(a2CC.x, a2CC.y, woCoating, randSampleBrdf);
			wi = mul(wi, surfaceDef.toCoatingLayerTangentSpace);
		}
	}
	else if(sampleLayer == LAYERIND_COATING_SHEEN)
	{
		if(woBase.y > 0)
		{
			wi = sampleSheen(surfaceDef.sheenRoughness, woBase, randSampleBrdf);
			wi = mul(wi, surfaceDef.toBaseLayerTangentSpace);
		}
	} 
	else if(sampleLayer == LAYERIND_SPEC_CONDUCTOR)
	{
		if(woBase.y > 0)
		{
			
			wi = sampleGGXReflectionConductor(a2.x, a2.y, woBase, randSampleBrdf);
			wi = mul(wi, surfaceDef.toBaseLayerTangentSpace);

		}
	}
	else if(sampleLayer == LAYERIND_SPEC_DIELECTRIC)
	{

		wi = sampleGGXReflectionDielectric(a2.x, a2.y, woBase, randSampleBrdf);
		wi = mul(wi, surfaceDef.toBaseLayerTangentSpace);
		
	} 
	else if(sampleLayer == LAYERIND_DIFFUSE_REFL)
	{
		if(woBase.y > 0)
		{
			wi = sampleDiffuseLambertian(a2.x, a2.y, woBase, randSampleBrdf);
			wi = mul(wi, surfaceDef.toBaseLayerTangentSpace);
		}
	} 
	else if(sampleLayer == LAYERIND_TRANSMITTED)
	{
		float fromIOR;
		float toIOR;
		bool exiting = !(HitKind() == HIT_KIND_TRIANGLE_FRONT_FACE);

		if(exiting)
		{
			fromIOR = rayState.getCurrentIOR();
			toIOR = rayState.getPreviousIOR();
		}
		else
		{
			fromIOR = rayState.getCurrentIOR();
			toIOR = surfaceDef.dielectricIOR;
		}

		if (hasDispersion(surfaceDef.flags))
		{
			if (exiting)
			{
				fromIOR = getRefractiveIndexForWavelength(surfaceDef.cauchysCoeffs, getHeroSpectralLambda());
			} 
			else
			{
				toIOR = getRefractiveIndexForWavelength(surfaceDef.cauchysCoeffs, getHeroSpectralLambda());
			}
		}


		float etaR = toIOR/fromIOR;
		
		wi = sampleGGXTransmitted(etaR, a2.x, a2.y, woBase, randSampleBrdf);
		wi = mul(wi, surfaceDef.toBaseLayerTangentSpace);
		
	} 

	//since the normal might not be the real geometry normal, could reflect ray inside object and assume reflection. Figure out better way to handle this later on.
	if(!allowTransmitted && (dot(surfaceDef.geometryNormal, wi) < 0))
	{
		wi = 0.f;
	}
	
	return wi;
}


template<typename RayState>
float3 getSampleDirection(in SurfaceDefinition surfaceDef, in RayState rayState, in PrecalculatedSurfaceData preCalcData, in float randMaterialType, in float2 randSampleBrdf, in float samplingProbabilities[LAYER_COUNT])
{
	return getSampleDirection(surfaceDef, rayState, preCalcData.woBase, preCalcData.woCoating, preCalcData.a2, randMaterialType, randSampleBrdf, samplingProbabilities);
}

template<typename RayState>
void evaluateSurface(in SurfaceDefinition surfaceDef,  in float3 woObjSpace, in float3 wiObjSpace,in float samplingProbabilities[LAYER_COUNT], in PrecalculatedSurfaceData precalculatedSurfData, in bool onlyPDF, inout RayState rayState, out SpectralSamples weightOut, out float pdfOut)
{
	float2 a2 = precalculatedSurfData.a2;

	float3 woCoating = precalculatedSurfData.woCoating;
	float3 woBase = precalculatedSurfData.woBase;

	bool exiting = precalculatedSurfData.exiting;
	float fromIOR = precalculatedSurfData.fromIOR;
	float toIOR = precalculatedSurfData.toIOR;

	SpectralSamples weightSum = (SpectralSamples)0.f;
	float pdfSum = 0.f;

	float3 wiCoating = mul(surfaceDef.toCoatingLayerTangentSpace, wiObjSpace);
	float3 wiBase = mul(surfaceDef.toBaseLayerTangentSpace, wiObjSpace);

	wiCoating = normalize(wiCoating);
	wiBase = normalize(wiBase);

	float energyLeft = 1.f;

	if (samplingProbabilities[LAYERIND_COATING_GGX] > 0)
	{
		float2 a2CC = calculateRoughnessParams(surfaceDef.clearCoatRoughness, 0.f);
		ReflectionDielectric coating = ReflectionDielectric::init(a2CC, surfaceDef.clearCoatRoughness, surfaceDef.clearCoatIOR / fromIOR);
		float pdf = coating.pdf(woCoating, wiCoating);

		if (pdf > 0 && !onlyPDF)
		{
			weightSum = weightSum + evaluateLayer(coating, woCoating, wiCoating, surfaceDef.clearCoatAmount * energyLeft);
			energyLeft *= coating.getEnergyLeftAfterLayer(woCoating, wiCoating, surfaceDef.clearCoatAmount);
			pdfSum += samplingProbabilities[LAYERIND_COATING_GGX] * pdf;
		}
	}

	if (samplingProbabilities[LAYERIND_COATING_SHEEN] > 0)
	{
		ReflectionSheen sheen = ReflectionSheen::init(surfaceDef.sheenColor, surfaceDef.sheenRoughness);
		float pdf = sheen.pdf(woBase, wiBase);

		if (pdf > 0 && !onlyPDF)
		{
			weightSum = weightSum + evaluateLayer(sheen, woBase, wiBase, surfaceDef.sheenAmount * energyLeft);
			energyLeft *= sheen.getEnergyLeftAfterLayer(woBase, wiBase, surfaceDef.sheenAmount);
			pdfSum += samplingProbabilities[LAYERIND_COATING_SHEEN] * pdf;
		}
	}

	if (samplingProbabilities[LAYERIND_SPEC_CONDUCTOR] > 0)
	{
		ReflectionConductor conductor = ReflectionConductor::init(surfaceDef.albedo, surfaceDef.specular, a2, surfaceDef.roughness, fromIOR);
		float pdf = conductor.pdf(woBase, wiBase);

		if (pdf > 0 && !onlyPDF)
		{
			weightSum = weightSum + evaluateLayer(conductor, woBase, wiBase, surfaceDef.metalness * energyLeft);
			energyLeft *= conductor.getEnergyLeftAfterLayer(woBase, wiBase, surfaceDef.metalness);
			pdfSum += samplingProbabilities[LAYERIND_SPEC_CONDUCTOR] * pdf;
		}
	}


	if (samplingProbabilities[LAYERIND_SPEC_DIELECTRIC] > 0)
	{
		ReflectionDielectric spec = ReflectionDielectric::init(a2, surfaceDef.roughness, toIOR / fromIOR);
		float pdf = spec.pdf(woBase, wiBase);

		if (pdf > 0 && !onlyPDF)
		{
			weightSum = weightSum + evaluateLayer(spec, woBase, wiBase, surfaceDef.specularAmount * energyLeft);
			energyLeft *= spec.getEnergyLeftAfterLayer(woBase, wiBase, surfaceDef.specularAmount);
			pdfSum += samplingProbabilities[LAYERIND_SPEC_DIELECTRIC] * pdf;
		}

	}

	if (samplingProbabilities[LAYERIND_DIFFUSE_REFL] > 0)
	{
		DiffuseLayer diff = DiffuseLayer::init(surfaceDef.albedo, a2);
		float pdf = diff.pdf(woBase, wiBase);

		if (pdf > 0 && !onlyPDF)
		{
			float diffuseAmount = 1.f - surfaceDef.transparency;
			weightSum = weightSum + evaluateLayer(diff, woBase, wiBase, diffuseAmount * energyLeft);
			energyLeft *= diff.getEnergyLeftAfterLayer(woBase, wiBase, diffuseAmount);
			pdfSum += samplingProbabilities[LAYERIND_DIFFUSE_REFL] * pdf;
		}

	}

	if (samplingProbabilities[LAYERIND_TRANSMITTED] > 0)
	{
		if (hasDispersion(surfaceDef.flags))
		{
			if (exiting)
			{
				fromIOR = getRefractiveIndexForWavelength(surfaceDef.cauchysCoeffs, getHeroSpectralLambda());
			}
			else
			{
				toIOR = getRefractiveIndexForWavelength(surfaceDef.cauchysCoeffs, getHeroSpectralLambda());
			}

		}

		float etaR = toIOR / fromIOR;

		TransmittedLayer transmitted = TransmittedLayer::init(a2, surfaceDef.roughness, etaR);
		float pdf = transmitted.pdf(woBase, wiBase);

		if (pdf > 0 && !onlyPDF)
		{

			pdfSum += samplingProbabilities[LAYERIND_TRANSMITTED] * pdf;

			bool twoSided = isTwoSided(surfaceDef.flags);
			bool wasTransmitted = (dot(surfaceDef.geometryNormal, wiObjSpace) * dot(surfaceDef.geometryNormal, woObjSpace) < 0.f) && !twoSided;

			float solidAngleCompression = 1.f;

			if (wasTransmitted)
			{
				//if transmitted, handle solid angle compression (btdf asymmetry)
				solidAngleCompression *= sqr(1.f / etaR);

				//went in or exited?
				if (exiting)
				{
					rayState.exitedVolume();

				}
				else
				{
					rayState.enteredVolume(surfaceDef.dielectricIOR, surfaceDef.absorption);
				}
			}

			weightSum = weightSum + evaluateLayer(transmitted, woBase, wiBase, surfaceDef.transparency * energyLeft) * solidAngleCompression;
			energyLeft *= transmitted.getEnergyLeftAfterLayer(woBase, wiBase, surfaceDef.transparency);

			if (hasDispersion(surfaceDef.flags))
			{
				rayState.setStateFlags(rayState.getStateFlags() | RAYSTATE_FLAGS_SECONDARY_LAMBDAS_TERMINATED);
			}
		}
	}


	pdfOut = pdfSum;
	weightOut = weightSum;
}

template<typename RayState>
void getPrecalculatedSurfaceData(in SurfaceDefinition surfaceDef, in RayState rayState, in float3 woObjSpace, in bool triangleHitFrontFace, out PrecalculatedSurfaceData surfaceDataOut)
{
	float2 a2 = calculateRoughnessParams(surfaceDef.roughness, surfaceDef.anisotropy);

	float3 woCoating = mul(surfaceDef.toCoatingLayerTangentSpace, woObjSpace);
	float3 woBase = mul(surfaceDef.toBaseLayerTangentSpace, woObjSpace);

	woCoating = normalize(woCoating);
	woBase = normalize(woBase);

	float fromIOR;
	float toIOR;
	bool exiting = !(triangleHitFrontFace);
	if (exiting)
	{
		fromIOR = rayState.getCurrentIOR();
		toIOR = rayState.getPreviousIOR();
	}
	else
	{
		fromIOR = rayState.getCurrentIOR();
		toIOR = surfaceDef.dielectricIOR;
	}


	surfaceDataOut.a2 = a2;
	surfaceDataOut.woBase = woBase;
	surfaceDataOut.woCoating = woCoating;
	surfaceDataOut.fromIOR = fromIOR;
	surfaceDataOut.toIOR = toIOR;
	surfaceDataOut.exiting = exiting;
}

template<typename RayState>
void evaluateSurfaceAndGenerateNextSampleDirection(in SurfaceDefinition surfaceDef, inout RayState rayState, in float3 rayDirObjSpace, in float4 randomSamples, in bool triangleHitFrontFace,  out SpectralSamples weightOut, out float3 nextSampleDirOut)
{
	PrecalculatedSurfaceData precalculatedSurfData;
	getPrecalculatedSurfaceData(surfaceDef, rayState, -rayDirObjSpace, triangleHitFrontFace, precalculatedSurfData);

	float samplingProbabilities[LAYER_COUNT];
	calculateNormalizedMaterialLayerSamplingProbabilities(surfaceDef, precalculatedSurfData, randomSamples.xy, samplingProbabilities);
	float3 wiObjSpace = getSampleDirection(surfaceDef, rayState, precalculatedSurfData, randomSamples.w, randomSamples.xy, samplingProbabilities);

	SpectralSamples weightSum = (SpectralSamples)0.f;
	float pdfSum = 0.f;

	if (!isZero(wiObjSpace))
	{
		evaluateSurface(surfaceDef, -rayDirObjSpace, wiObjSpace, samplingProbabilities, precalculatedSurfData, false, rayState, weightSum, pdfSum);

		if (pdfSum > 0.f)
		{
			weightSum = weightSum / pdfSum;
		}
	}

	weightOut = weightSum;
	nextSampleDirOut = wiObjSpace;
}


#endif