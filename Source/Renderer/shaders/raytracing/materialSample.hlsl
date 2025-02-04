#ifndef MATERIALSAMPLE_HLSL_INCL
#define MATERIALSAMPLE_HLSL_INCL
#include "bsdfSample.hlsl"

#define LAYERIND_COATING_GGX 0
#define LAYERIND_COATING_SHEEN 1
#define LAYERIND_SPEC_CONDUCTOR 2
#define LAYERIND_SPEC_DIELECTRIC 3
#define LAYERIND_DIFFUSE_REFL 4
#define LAYERIND_TRANSMITTED 5

#define LAYER_COUNT (LAYERIND_TRANSMITTED + 1)

#define MAT_SAMPLING_ADD_LAYER(layer, probability) samplingProbabilities[layer] = (probability); sampleSum += (probability);

void calculateNormalizedMaterialLayerSamplingProbabilities(in SurfaceDefinition surfaceDef, in Payload payload, 
	in float3 woBase,in float3 woCoating, in float fromIOR, in float toIOR, in float2 a2, in float4 rand, 
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

float3 getSampleDirection(in SurfaceDefinition surfaceDef, in Payload payload, in float3 woBase, in float3 woCoating, in float2 a2, in float4 rand, in float samplingProbabilities[LAYER_COUNT])
{	
	float materialTypeRand = max(0, rand.w + 0.00001f);
	float3 randSampleBrdf = rand.xyz;
	
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
			fromIOR = payloadGetCurrentIOR(payload); 
			toIOR = payloadGetBeforeCurrentIOR(payload); 
		}
		else
		{
			fromIOR = payloadGetCurrentIOR(payload); 
			toIOR = surfaceDef.dielectricIOR;
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


void sampleMaterial(in SurfaceDefinition surfaceDef, inout Payload payload, in float3 rayDirObjSpace,  out float3 weightOut, out float3 nextSampleDirOut)
{
	uint sampleIndex = g_currentRandomSampleIndex + payload.pathLength * 7 + payload.rayIndex * 11;
	float4 randomSamples = getRandomSampleFloat4(sampleIndex);
	float2 a2 = calculateRoughnessParams(surfaceDef.roughness, surfaceDef.anisotropy);
	
	float3 woCoating = mul(surfaceDef.toCoatingLayerTangentSpace, -rayDirObjSpace);
	float3 woBase = mul(surfaceDef.toBaseLayerTangentSpace, -rayDirObjSpace);
	
	woCoating = normalize(woCoating);
	woBase = normalize(woBase);
	
	float fromIOR;
	float toIOR;
	bool exiting = !(HitKind() == HIT_KIND_TRIANGLE_FRONT_FACE);
	if(exiting)
	{
		fromIOR = payloadGetCurrentIOR(payload); 
		toIOR = payloadGetBeforeCurrentIOR(payload); 
	}
	else
	{
		fromIOR = payloadGetCurrentIOR(payload); 
		toIOR = surfaceDef.dielectricIOR;
	}

	float samplingProbabilities[LAYER_COUNT];
	calculateNormalizedMaterialLayerSamplingProbabilities(surfaceDef, payload, woBase, woCoating, fromIOR, toIOR, a2,randomSamples, samplingProbabilities);
	float3 wiObjSpace = getSampleDirection(surfaceDef, payload, woBase, woCoating, a2, randomSamples, samplingProbabilities);
	
	float3 weightSum = 0.f;
	float pdfSum = 0.f;
	
	if(isZero(wiObjSpace))
	{
		nextSampleDirOut = wiObjSpace;
		weightOut = weightSum;
		return;
	}
	
	float3 wiCoating = mul(surfaceDef.toCoatingLayerTangentSpace, wiObjSpace);
	float3 wiBase = mul(surfaceDef.toBaseLayerTangentSpace, wiObjSpace);
	
	wiCoating = normalize(wiCoating);
	wiBase = normalize(wiBase);
	
	float3 energyLeft = 1.f;
	
	if(samplingProbabilities[LAYERIND_COATING_GGX] > 0)
	{
		if(onSameHemisphere(woCoating, wiCoating))
		{
			float etaR = surfaceDef.clearCoatIOR / fromIOR;

			//singleScatter
			float2 a2CC = calculateRoughnessParams(surfaceDef.clearCoatRoughness, 0.f);
			
			float pdf = pdfGGXReflectionDielectric(woCoating, wiCoating, a2CC.x, a2CC.y);
			float3 weight = evaluateGGXReflectionDielectric(etaR, a2CC.x, a2CC.y, woCoating, wiCoating);
			
			//multiscatter
			float3 wm = normalize(woCoating + wiCoating);
			float3 fms = getFmsDielectric(etaR, dot(woCoating, wm));
			float3 msbrdf = getEnergyCompensation(fms, woCoating.y, wiCoating.y, surfaceDef.clearCoatRoughness, weight);
			
			weight += msbrdf;
			weight *= surfaceDef.clearCoatAmount;
			
			if(pdf > 0)
			{
				weightSum += weight * energyLeft * abs(wiCoating.y);
				pdfSum += samplingProbabilities[LAYERIND_COATING_GGX] * pdf;
			}
			
			float amountOfEnergyAfterSpecular = getEnergyRemainingAfterSpecular(etaR, woCoating.y, wiCoating.y, surfaceDef.clearCoatRoughness, surfaceDef.clearCoatAmount);
			energyLeft *= amountOfEnergyAfterSpecular;
		}
	}
	
	if(samplingProbabilities[LAYERIND_COATING_SHEEN] > 0)
	{
		if(onSameHemisphere(woBase, wiBase))
		{
			//singleScatter
			float pdf = pdfSheen(woBase, wiBase, surfaceDef.sheenRoughness);
			float3 weight = evaluateSheen(surfaceDef.sheenColor, surfaceDef.sheenRoughness, woBase, wiBase);
			weight *= surfaceDef.sheenAmount;

			if(pdf > 0)
			{
				weightSum += weight * energyLeft * abs(wiBase.y);
				pdfSum += samplingProbabilities[LAYERIND_COATING_SHEEN] * pdf;
			}
			energyLeft *= getEnergyRemainingAfterSheen(woBase.y, wiBase.y, surfaceDef.sheenRoughness, surfaceDef.sheenAmount);
			
		}
	}
	
	if(samplingProbabilities[LAYERIND_SPEC_CONDUCTOR] > 0)
	{
		if(onSameHemisphere(woBase, wiBase))
		{
			float2 r = getConductorRefractiveIndexAndExtinctionthroughputSquared(surfaceDef.albedo.r, surfaceDef.specular.r);
			float2 g = getConductorRefractiveIndexAndExtinctionthroughputSquared(surfaceDef.albedo.g, surfaceDef.specular.g);
			float2 b = getConductorRefractiveIndexAndExtinctionthroughputSquared(surfaceDef.albedo.b, surfaceDef.specular.b);
		
			float3 n = float3(r.x, g.x, b.x);
			float3 k = float3(r.y, g.y, b.y);
			k = sqrt(k);
			
			float3 etaR = n / fromIOR;
			float3 etaK = k / fromIOR;
			
			//single scatter
			float pdf = pdfGGXReflectionConductor(woBase, wiBase, a2.x, a2.y);
			float3 weight = evaluateGGXReflectionConductor(etaR, etaK, a2.x, a2.y, woBase, wiBase);
			
			//multiscatter
			float3 wm = normalize(woBase + wiBase);
			float3 fms = getFmsConductor(etaR, etaK, dot(woBase, wm));
			float3 msbrdf = getEnergyCompensation(fms, woBase.y, wiBase.y, surfaceDef.roughness, weight);
			weight += msbrdf;
			weight *= surfaceDef.metalness;
			
			if(pdf > 0)
			{
				weightSum += weight * energyLeft * abs(wiBase.y);
				pdfSum += samplingProbabilities[LAYERIND_SPEC_CONDUCTOR] * pdf;
			}
			
			energyLeft *= 1.f - surfaceDef.metalness;
		}
	} 
	
	
	if(samplingProbabilities[LAYERIND_SPEC_DIELECTRIC] > 0)
	{
		//the ray could also be coming from inside the object, need to take reflection into account in that case (translucent)
		if(onSameHemisphere(woBase, wiBase))
		{
			bool flippedWOBase = false;
			if(woBase.y < 0)
			{
				flippedWOBase = true;
				woBase.y = -woBase.y;
				wiBase.y = -wiBase.y;
			}
			
			float etaR = toIOR/fromIOR;

			float pdf = pdfGGXReflectionDielectric(woBase, wiBase, a2.x, a2.y);
			float3 weight = evaluateGGXReflectionDielectric(etaR, a2.x, a2.y, woBase, wiBase);
			
	
			//multiscatter
			float3 wm = normalize(woBase + wiBase);
			float3 fms = getFmsDielectric(etaR, dot(woBase, wm));
			float3 msbrdf = getEnergyCompensation(fms, woBase.y, wiBase.y, surfaceDef.roughness, weight);
			weight += msbrdf;
			weight *= surfaceDef.specularAmount * (1.f - surfaceDef.metalness);
			
			if(pdf > 0)
			{
				weightSum += weight * energyLeft * abs(wiBase.y);
				pdfSum += samplingProbabilities[LAYERIND_SPEC_DIELECTRIC] * pdf;
			}
			
			float amountOfEnergyAfterSpecular = getEnergyRemainingAfterSpecular(etaR, woBase.y, wiBase.y, surfaceDef.roughness, surfaceDef.specularAmount);
			energyLeft *= amountOfEnergyAfterSpecular;
			
			if(flippedWOBase)
			{
				woBase.y = -woBase.y;
				wiBase.y = -wiBase.y;
			}
		}
		
	}
	
	if(samplingProbabilities[LAYERIND_DIFFUSE_REFL] > 0)
	{
		if(onSameHemisphere(woBase, wiBase))
		{
			float diffuseAmount = 1.f - surfaceDef.transparency; 
			
			float pdf = pdfDiffuseLambertian(woBase, wiBase, a2.x, a2.y);
			float3 weight = evaluateDiffuseLambertian(surfaceDef.albedo, a2.x, a2.y, woBase, wiBase);
			weight *= diffuseAmount;

			if(pdf > 0)
			{
				weightSum += weight * energyLeft * abs(wiBase.y);
				pdfSum += samplingProbabilities[LAYERIND_DIFFUSE_REFL] * pdf;
			}
			
			
			energyLeft *= 1.f - diffuseAmount;  
		}
		
	}

	if(samplingProbabilities[LAYERIND_TRANSMITTED] > 0)
	{
		if(!onSameHemisphere(woBase, wiBase))
		{
			float etaR = toIOR/fromIOR;
	
			float pdf = pdfGGXTransmitted(etaR, woBase, wiBase, a2.x, a2.y);
			float3 weight = evaluateGGXTransmitted(etaR, a2.x, a2.y, woBase, wiBase);
			
			float3 msbrdf = getEnergyCompensationTranslucent(etaR, woBase.y, wiBase.y, surfaceDef.roughness, weight);
			weight += msbrdf;
			
			bool wasTransmitted = (dot(surfaceDef.geometryNormal, wiObjSpace) * dot(surfaceDef.geometryNormal, -rayDirObjSpace) < 0.f) && !surfaceDef.isTwoSided;

			if(wasTransmitted)
			{
				//if transmitted, handle solid angle compression (btdf asymmetry)
				weight *= sqr(1.f/etaR);
				
				//went in or exited?
				if(exiting)
				{
					payloadRayExitedVolume(payload);
					
				}
				else
				{
					payloadRayEnteredVolume(payload, surfaceDef.dielectricIOR);
					payload.absorption = surfaceDef.absorption;
				}
			}

			if(pdf > 0.f)
			{
				weightSum += weight * energyLeft * abs(wiBase.y) * surfaceDef.transparency;
				pdfSum += samplingProbabilities[LAYERIND_TRANSMITTED] * pdf;
			}
		}
		
		
	}
	
	if(pdfSum > 0.f)
	{
		weightSum /= pdfSum;
	}
	
	nextSampleDirOut = wiObjSpace;
	weightOut = weightSum;
}


#endif