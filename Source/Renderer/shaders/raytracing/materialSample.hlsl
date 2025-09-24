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
    float3 coatingLayerNormal;
    float3 baseLayerNormal;
	float3 woBase;
    float fromIOR;
	float3 woCoating;
	float toIOR;
    float2 a2;
	bool exiting;
    
    
    float3x3 toCoatingLayerTangentSpace(float3 tangent)
    {
        return constructBasisTransform(coatingLayerNormal, tangent);

    }
	
    float3x3 toBaseLayerTangentSpace(float3 tangent)
    {
        return constructBasisTransform(baseLayerNormal, tangent);

    }
};

typedef uint TransmissionType;

#define TRANSMISSION_TYPE_NONE 0
#define TRANSMISSION_TYPE_ENTERED 1
#define TRANSMISSION_TYPE_EXITED 2
#define TRANSMISSION_TYPE_DISPERSED 4

#define EVALUATE_FLAGS_NONE 0
#define EVALUATE_FLAGS_PDF_ONLY 1
#define EVALUATE_FLAGS_LIGHT_PATH 2

void calculateNormalizedMaterialLayerSamplingProbabilities(in SurfaceDefinition surfaceDef,
	in float3 woBase, in float3 woCoating, in float fromIOR, in float toIOR, in float2 a2, out float samplingProbabilities[LAYER_COUNT])
{
    bool comingFromInside = woBase.y < 0;
    float fromInsideMultiplier = comingFromInside ? 0.f : 1.f;
    float ratioRefr = 1.f - getAvgFresnel(toIOR/fromIOR); 
    float dielAmount = 1.f - surfaceDef.metalness;
    float refrProb = ratioRefr * surfaceDef.transparency * dielAmount;
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
	MAT_SAMPLING_ADD_LAYER(LAYERIND_SPEC_DIELECTRIC, dielAmount * surfaceDef.specularAmount * reflProb);
	MAT_SAMPLING_ADD_LAYER(LAYERIND_DIFFUSE_REFL, dielAmount * (1.f - surfaceDef.transparency) * reflProb * fromInsideMultiplier);
	
	MAT_SAMPLING_ADD_LAYER(LAYERIND_TRANSMITTED, dielAmount * refrProb);
	
    sampleSum = max(0.000001f, sampleSum);
	
    for (uint i = 0; i < LAYER_COUNT; ++i)
    {
        samplingProbabilities[i] /= sampleSum;
    }

}

void calculateNormalizedMaterialLayerSamplingProbabilities(in SurfaceDefinition surfaceDef, in PrecalculatedSurfaceData preCalcData,  out float samplingProbabilities[LAYER_COUNT])
{
    calculateNormalizedMaterialLayerSamplingProbabilities(surfaceDef, preCalcData.woBase, preCalcData.woCoating, preCalcData.fromIOR, preCalcData.toIOR, preCalcData.a2, samplingProbabilities);
}


float3 getSampleDirectionOS(in SurfaceDefinition surfaceDef, in PrecalculatedSurfaceData preCalcData, in float randMaterialType, in float2 randSampleBrdf, in float samplingProbabilities[LAYER_COUNT])
{	
	float materialTypeRand = max(0, randMaterialType + 0.00001f);
	
	float3 wi = 0;
	float allowTransmitted = surfaceDef.transparency > 0;
	
    float fromIOR = preCalcData.fromIOR;
    float toIOR = preCalcData.toIOR;
    float3 woBase = preCalcData.woBase;
    float3 woCoating = preCalcData.woCoating;
    float2 a2 = preCalcData.a2;
    bool exiting = preCalcData.exiting;
    
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
            wi = sampleGGXReflectionDielectric(a2CC.x, a2CC.y, preCalcData.woCoating, randSampleBrdf);
            wi = mul(wi, preCalcData.toCoatingLayerTangentSpace(surfaceDef.tangent));
        }
	}
	else if(sampleLayer == LAYERIND_COATING_SHEEN)
	{
		if(woBase.y > 0)
		{
			wi = sampleSheen(surfaceDef.sheenRoughness, woBase, randSampleBrdf);
            wi = mul(wi, preCalcData.toBaseLayerTangentSpace(surfaceDef.tangent));
        }
	} 
	else if(sampleLayer == LAYERIND_SPEC_CONDUCTOR)
	{
		if(woBase.y > 0)
		{
			
			wi = sampleGGXReflectionConductor(a2.x, a2.y, woBase, randSampleBrdf);
            wi = mul(wi, preCalcData.toBaseLayerTangentSpace(surfaceDef.tangent));

		}
	}
	else if(sampleLayer == LAYERIND_SPEC_DIELECTRIC)
	{
        
        wi = sampleGGXReflectionDielectric(a2.x, a2.y, woBase, randSampleBrdf);
        wi = mul(wi, preCalcData.toBaseLayerTangentSpace(surfaceDef.tangent));
        
    } 
	else if(sampleLayer == LAYERIND_DIFFUSE_REFL)
	{
		if(woBase.y > 0)
		{
			wi = sampleDiffuseLambertian(a2.x, a2.y, woBase, randSampleBrdf);
            wi = mul(wi, preCalcData.toBaseLayerTangentSpace(surfaceDef.tangent));
        }
	} 
	else if(sampleLayer == LAYERIND_TRANSMITTED)
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


		float etaR = toIOR/fromIOR;
		
		wi = sampleGGXTransmitted(etaR, a2.x, a2.y, woBase, randSampleBrdf);
        wi = mul(wi, preCalcData.toBaseLayerTangentSpace(surfaceDef.tangent));
		
	} 

	//since the normal might not be the real geometry normal, could reflect ray inside object and assume reflection. Figure out better way to handle this later on.
	if(!allowTransmitted && (dot(surfaceDef.geometryNormal, wi) < 0))
	{
		wi = 0.f;
	}
	
	return wi;
}

void evaluateSurface(in SurfaceDefinition surfaceDef, in float3 woObjSpace, in float3 wiObjSpace, in float samplingProbabilities[LAYER_COUNT], in PrecalculatedSurfaceData precalculatedSurfData, in uint evaluateFlags, out SpectralSamples weightOut, out float pdfOut, out TransmissionType transmissionTypeOut)
{
	float2 a2 = precalculatedSurfData.a2;

	float3 woCoating = precalculatedSurfData.woCoating;
	float3 woBase = precalculatedSurfData.woBase;

	bool exiting = precalculatedSurfData.exiting;
	float fromIOR = precalculatedSurfData.fromIOR;
	float toIOR = precalculatedSurfData.toIOR;

	SpectralSamples weightSum = (SpectralSamples)0.f;
	float pdfSum = 0.f;

    float3 wiCoating = mul(precalculatedSurfData.toCoatingLayerTangentSpace(surfaceDef.tangent), wiObjSpace);
    float3 wiBase = mul(precalculatedSurfData.toBaseLayerTangentSpace(surfaceDef.tangent), wiObjSpace);

	wiCoating = normalize(wiCoating);
	wiBase = normalize(wiBase);

    transmissionTypeOut = TRANSMISSION_TYPE_NONE;
    
    bool onlyPDF = (evaluateFlags &  EVALUATE_FLAGS_PDF_ONLY) != 0;
    bool lightPath = (evaluateFlags & EVALUATE_FLAGS_LIGHT_PATH) != 0;
	
	
    bool baseReflected = dot(woObjSpace, surfaceDef.geometryNormal) * dot(wiObjSpace, surfaceDef.geometryNormal) > 0;
    bool baseRefracted = dot(woObjSpace, surfaceDef.geometryNormal) * dot(wiObjSpace, surfaceDef.geometryNormal) < 0;
	float energyLeft = 1.f;
    float energyLeftFromLayer = 1.f;
	
	//coating layer (ggx & sheen)
	{
        if (samplingProbabilities[LAYERIND_COATING_GGX] > 0)
        {
            float2 a2CC = calculateRoughnessParams(surfaceDef.clearCoatRoughness, 0.f);
            ReflectionDielectric coating = ReflectionDielectric::init(a2CC, surfaceDef.clearCoatRoughness, surfaceDef.clearCoatIOR / fromIOR);
            float pdf = coating.pdf(woCoating, wiCoating);

            bool coatingReflected = dot(woObjSpace, surfaceDef.geometryNormal) * dot(wiObjSpace, surfaceDef.geometryNormal) > 0;
		
            if (pdf > 0 && coatingReflected)
            {
                if (!onlyPDF)
                {
                    weightSum = weightSum + evaluateLayer(coating, woCoating, wiCoating, surfaceDef.clearCoatAmount * energyLeft);
                    energyLeftFromLayer *= coating.getEnergyLeftAfterLayer(woCoating, wiCoating, surfaceDef.clearCoatAmount);
                }
                
                pdfSum += samplingProbabilities[LAYERIND_COATING_GGX] * pdf;
            }
        }

        if (samplingProbabilities[LAYERIND_COATING_SHEEN] > 0)
        {
            ReflectionSheen sheen = ReflectionSheen::init(surfaceDef.sheenColor, surfaceDef.sheenRoughness);
            float pdf = sheen.pdf(woBase, wiBase);

            if (pdf > 0 &&  baseReflected)
            {
                if (!onlyPDF)
                {
                    weightSum = weightSum + evaluateLayer(sheen, woBase, wiBase, surfaceDef.sheenAmount * energyLeft);
                    energyLeftFromLayer *= sheen.getEnergyLeftAfterLayer(woBase, wiBase, surfaceDef.sheenAmount);
                }
                pdfSum += samplingProbabilities[LAYERIND_COATING_SHEEN] * pdf;
            }
        }
    }
	

	//reduce energy from coating
    energyLeft *= energyLeftFromLayer;
    energyLeftFromLayer = 1.f;
	
	
    float dielAmount = 1.f - surfaceDef.metalness;
	
	//spec layer dielelectric & conductor
	{
        if (samplingProbabilities[LAYERIND_SPEC_CONDUCTOR] > 0)
        {
            ReflectionConductor conductor = ReflectionConductor::init(surfaceDef.albedo, surfaceDef.specular, a2, surfaceDef.roughness, fromIOR);
            float pdf = conductor.pdf(woBase, wiBase);

            if (pdf > 0 && baseReflected)
            {
                if (!onlyPDF)
                {
                    weightSum = weightSum + evaluateLayer(conductor, woBase, wiBase, surfaceDef.metalness * energyLeft);
                    energyLeftFromLayer *= conductor.getEnergyLeftAfterLayer(woBase, wiBase, surfaceDef.metalness);
                }
                pdfSum += samplingProbabilities[LAYERIND_SPEC_CONDUCTOR] * pdf;
            }
        }


        if (samplingProbabilities[LAYERIND_SPEC_DIELECTRIC] > 0)
        {
            ReflectionDielectric spec = ReflectionDielectric::init(a2, surfaceDef.roughness, toIOR / fromIOR);
            float pdf = spec.pdf(woBase, wiBase);

            if (pdf > 0 && baseReflected)
            {
                if (!onlyPDF)
                {
                    float specAmount = dielAmount * surfaceDef.specularAmount;
                    weightSum = weightSum + evaluateLayer(spec, woBase, wiBase, specAmount * energyLeft);
                    energyLeftFromLayer *= spec.getEnergyLeftAfterLayer(woBase, wiBase, specAmount);
                }
                pdfSum += samplingProbabilities[LAYERIND_SPEC_DIELECTRIC] * pdf;
            }

        }
    }
	
    energyLeft *= energyLeftFromLayer;
    energyLeftFromLayer = 1.f;

	//bottom layer (diffuse and transmittance)
	{
        if (samplingProbabilities[LAYERIND_DIFFUSE_REFL] > 0)
        {
            DiffuseLayer diff = DiffuseLayer::init(surfaceDef.albedo, a2);
            float pdf = diff.pdf(woBase, wiBase);

            if (pdf > 0 && baseReflected)
            {
                if (!onlyPDF)
                {
                    float diffuseAmount = (1.f - surfaceDef.transparency) * dielAmount;
                    weightSum = weightSum + evaluateLayer(diff, woBase, wiBase, diffuseAmount * energyLeft);
                    energyLeftFromLayer *= diff.getEnergyLeftAfterLayer(woBase, wiBase, diffuseAmount);
                }
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
                transmissionTypeOut |= TRANSMISSION_TYPE_DISPERSED;
            }

            float etaR = toIOR / fromIOR;

            TransmittedLayer transmitted = TransmittedLayer::init(a2, surfaceDef.roughness, etaR);
            float pdf = transmitted.pdf(woBase, wiBase);

            if (pdf > 0  && baseRefracted)
            {

                pdfSum += samplingProbabilities[LAYERIND_TRANSMITTED] * pdf;

                bool twoSided = isTwoSided(surfaceDef.flags);
                bool wasTransmitted = !twoSided;

                float solidAngleCompression = 1.f;

			
                if (wasTransmitted)
                {
				    //if transmitted, handle solid angle compression (btdf asymmetry), but only for camera path
                    if (!lightPath)
                    {
                        solidAngleCompression *= sqr(1.f / etaR);
                    }
				    
                    transmissionTypeOut |= exiting ? TRANSMISSION_TYPE_EXITED : TRANSMISSION_TYPE_ENTERED;

                }
                else if (twoSided)
                {
                    transmissionTypeOut |= TRANSMISSION_TYPE_ENTERED | TRANSMISSION_TYPE_EXITED;

                }

                float transparencyAmount = surfaceDef.transparency * dielAmount;
                if (!onlyPDF)
                {
                    weightSum = weightSum + evaluateLayer(transmitted, woBase, wiBase, transparencyAmount * energyLeft) * solidAngleCompression;
                    energyLeftFromLayer *= transmitted.getEnergyLeftAfterLayer(woBase, wiBase, transparencyAmount);
                }
            }
        }
    }
	
	
	pdfOut = pdfSum;
	weightOut = weightSum;
}

void getPrecalculatedSurfaceData(in SurfaceDefinition surfaceDef, in float currentIOR, in float previousIOR, in float3 woObjSpace, in bool triangleHitFrontFace, out PrecalculatedSurfaceData surfaceDataOut)
{
	float2 a2 = calculateRoughnessParams(surfaceDef.roughness, surfaceDef.anisotropy);
	float fromIOR;
	float toIOR;
	bool exiting = !(triangleHitFrontFace);
	if (exiting)
	{
        fromIOR = currentIOR;
        toIOR = previousIOR;
    }
	else
	{
        fromIOR = currentIOR;
		toIOR = surfaceDef.dielectricIOR;
	}
	surfaceDataOut.a2 = a2;
	surfaceDataOut.fromIOR = fromIOR;
	surfaceDataOut.toIOR = toIOR;
	surfaceDataOut.exiting = exiting;
    
    surfaceDataOut.baseLayerNormal = surfaceDef.baseLayerNormal;
    surfaceDataOut.coatingLayerNormal = surfaceDataOut.coatingLayerNormal;
    

    //make sure the shdading normals don't point to wrong direction
    //if (dot(surfaceDataOut.baseLayerNormal, woObjSpace) < GLANCING_ANGLE_EPSILON)
    {
        //surfaceDataOut.baseLayerNormal = reflect(-surfaceDataOut.baseLayerNormal, surfaceDef.geometryNormal);
        //surfaceDataOut.baseLayerNormal = surfaceDef.geometryNormal;
        surfaceDataOut.baseLayerNormal = nudgeNormal(-woObjSpace, surfaceDataOut.baseLayerNormal, surfaceDef.roughness, surfaceDef.transparency, isTwoSided(surfaceDef.flags));

    }
    
    //if (dot(surfaceDataOut.coatingLayerNormal, woObjSpace) < GLANCING_ANGLE_EPSILON)
    {
        //surfaceDataOut.coatingLayerNormal = reflect(-surfaceDataOut.coatingLayerNormal, surfaceDef.geometryNormal);
        //surfaceDataOut.coatingLayerNormal = surfaceDef.geometryNormal;
        surfaceDataOut.coatingLayerNormal = nudgeNormal(-woObjSpace, surfaceDataOut.coatingLayerNormal, surfaceDef.clearCoatRoughness, surfaceDef.transparency, isTwoSided(surfaceDef.flags));
    }
    
    float3 woCoating = mul(surfaceDataOut.toCoatingLayerTangentSpace(surfaceDef.tangent), woObjSpace);
    float3 woBase = mul(surfaceDataOut.toBaseLayerTangentSpace(surfaceDef.tangent), woObjSpace);
    woCoating = normalize(woCoating);
    woBase = normalize(woBase);
    surfaceDataOut.woBase = woBase;
    surfaceDataOut.woCoating = woCoating;
}


#endif