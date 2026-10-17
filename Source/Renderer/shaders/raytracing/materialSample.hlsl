#ifndef MATERIALSAMPLE_HLSL_INCL
#define MATERIALSAMPLE_HLSL_INCL

#include "../materials/materialsCommonResources.hlsl"
#include "spectralDistribution.hlsl"
#include "../materials/materialLayers.hlsl"

#define LAYERIND_COATING_GGX 0
#define LAYERIND_COATING_SHEEN 1
#define LAYERIND_SPEC_CONDUCTOR 2
#define LAYERIND_SPEC_DIELECTRIC 3
#define LAYERIND_DIFFUSE_REFL 4
//subsurface is not a local (wo, wi) lobe: it is only selected through sampleScatteringEvent and never contributes to evaluateSurface
#define LAYERIND_SUBSURFACE 5


#define LAYER_COUNT (LAYERIND_SUBSURFACE + 1)

//Pulls the shading normals away from glancing angles so the sampled direction is less likely to disagree with the geometry normal.
//KEEP THIS OFF: it breaks the white furnace test, transmissive materials lose noticeable energy and the loss grows with roughness.
//#define NUDGE_SHADING_NORMALS
#define REJECT_SHADING_GEOMETRY_FRAME_MISMATCH

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

#define EVALUATE_FLAGS_NONE (1 << 0)
#define EVALUATE_FLAGS_PDF_ONLY (1 << 1)
#define EVALUATE_FLAGS_LIGHT_PATH (1 << 2)
#define EVALUATE_FLAGS_TREAT_AS_DELTA (1 << 3)
#define EVALUATE_FLAGS_DONT_PROJECT_SOLID_ANGLE (1 << 4)

struct SurfaceSamplingContext
{
    SurfaceDefinition surfaceDef;
    PrecalculatedSurfaceData precalculatedSurfaceData;
    float samplingProbabilities[LAYER_COUNT];
};

struct SurfaceEvaluation
{
    SpectralSamples weight;
    float pdf;
    TransmissionType transmissionType;
};

bool isConsideredDiffuseGGX(float2 roughness)
{
    return min(roughness.x, roughness.y) > 0.05f;
}

bool isConsideredDiffuseSheen(float roughness)
{
    return roughness > 0.15f;
}


void calculateNormalizedMaterialLayerSamplingProbabilities(in SurfaceDefinition surfaceDef,
	in float3 woBase, in float3 woCoating, in float fromIOR, in float toIOR, in float2 a2, out float samplingProbabilities[LAYER_COUNT])
{
    bool comingFromInside = woBase.y < 0;
    float fromInsideMultiplier = comingFromInside ? 0.f : 1.f;
    float dielAmount = 1.f - surfaceDef.metalness;
    float opaqueDielAmount = dielAmount * (1.f - surfaceDef.transparency);
	
	//explicit TIR handling (enable if Fresnel doesn't take TIR into account)
	/*float3 wi = refract(-woBase, wm, fromIOR/toIOR);
	if(isZero(wi))
	{
		reflProb = 1.f;
		refrProb = 0.f;
	}*/
	
    

    float sampleSum = 0.f;
	MAT_SAMPLING_ADD_LAYER(LAYERIND_COATING_GGX, surfaceDef.clearCoatAmount * fromInsideMultiplier);
	MAT_SAMPLING_ADD_LAYER(LAYERIND_COATING_SHEEN, surfaceDef.sheenAmount * fromInsideMultiplier);
	MAT_SAMPLING_ADD_LAYER(LAYERIND_SPEC_CONDUCTOR, surfaceDef.metalness * fromInsideMultiplier);
	MAT_SAMPLING_ADD_LAYER(LAYERIND_SPEC_DIELECTRIC, dielAmount * (surfaceDef.specularAmount + surfaceDef.transparency));
	MAT_SAMPLING_ADD_LAYER(LAYERIND_DIFFUSE_REFL, opaqueDielAmount * (1.f - surfaceDef.subsurfaceAmount) * fromInsideMultiplier);
	MAT_SAMPLING_ADD_LAYER(LAYERIND_SUBSURFACE, opaqueDielAmount * surfaceDef.subsurfaceAmount * fromInsideMultiplier);

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


uint selectMaterialLayer(in float randMaterialType, in float samplingProbabilities[LAYER_COUNT])
{
	float materialTypeRand = max(0, randMaterialType + 0.00001f);
	
	uint sampleLayer;
	for(sampleLayer = 0; sampleLayer < LAYER_COUNT; ++sampleLayer)
	{
		if(materialTypeRand < samplingProbabilities[sampleLayer] && samplingProbabilities[sampleLayer] > 0)
		{
			break;
		}
		materialTypeRand -= samplingProbabilities[sampleLayer];
	}
	return sampleLayer;
}

//samples a direction from the selected local layer. LAYERIND_SUBSURFACE (or no layer) returns a zero direction
float3 getSampleDirectionOSForLayer(in SurfaceSamplingContext surface, in uint sampleLayer, in float randbsdfSelect, in float2 randSampleBrdf, out bool isDiffuseBounceOut, out bool isDeltaDistributionOut)
{
	SurfaceDefinition surfaceDef = surface.surfaceDef;
	PrecalculatedSurfaceData preCalcData = surface.precalculatedSurfaceData;
	float3 wi = 0;
	float allowTransmitted = surfaceDef.transparency > 0;
	
    float fromIOR = preCalcData.fromIOR;
    float toIOR = preCalcData.toIOR;
    float3 woBase = preCalcData.woBase;
    float3 woCoating = preCalcData.woCoating;
    float2 a2 = preCalcData.a2;
    bool exiting = preCalcData.exiting;
    bool isDiffuseBounce = false;
    bool isDeltaDistribution = false;
	
	if(sampleLayer == LAYERIND_COATING_GGX)
	{
		if(woCoating.y > 0)
		{
			float2 a2CC = calculateRoughnessParams(surfaceDef.clearCoatRoughness, 0.f);
            wi = sampleGGXReflectionDielectric(a2CC.x, a2CC.y, preCalcData.woCoating, randSampleBrdf);
            wi = mul(wi, preCalcData.toCoatingLayerTangentSpace(surfaceDef.tangent));
            isDiffuseBounce = isConsideredDiffuseGGX(a2CC);
            isDeltaDistribution = isDeltaGGX(a2CC);

        }
	}
	else if(sampleLayer == LAYERIND_COATING_SHEEN)
	{
		if(woBase.y > 0)
		{
			wi = sampleSheen(surfaceDef.sheenRoughness, woBase, randSampleBrdf);
            wi = mul(wi, preCalcData.toBaseLayerTangentSpace(surfaceDef.tangent));
            isDiffuseBounce = isConsideredDiffuseSheen(surfaceDef.sheenRoughness);
            isDeltaDistribution = isDeltaSheen(surfaceDef.sheenRoughness);
        }
	} 
	else if(sampleLayer == LAYERIND_SPEC_CONDUCTOR)
	{
		if(woBase.y > 0)
		{
			
			wi = sampleGGXReflectionConductor(a2.x, a2.y, woBase, randSampleBrdf);
            wi = mul(wi, preCalcData.toBaseLayerTangentSpace(surfaceDef.tangent));
            isDiffuseBounce = isConsideredDiffuseGGX(a2);
            isDeltaDistribution = isDeltaGGX(a2);

		}
	}
	else if(sampleLayer == LAYERIND_SPEC_DIELECTRIC)
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
        SpecularLayer specLayer = SpecularLayer::init(a2, surfaceDef.roughness, etaR, surfaceDef.specularAmount > 0, surfaceDef.transparency > 0);

        wi = specLayer.sampleWi(woBase, randSampleBrdf, randbsdfSelect);
        wi = mul(wi, preCalcData.toBaseLayerTangentSpace(surfaceDef.tangent));
        isDiffuseBounce = isConsideredDiffuseGGX(a2);
        isDeltaDistribution = isDeltaGGX(a2);
        
    } 
	else if(sampleLayer == LAYERIND_DIFFUSE_REFL)
	{
		if(woBase.y > 0)
		{
			wi = sampleDiffuseLambertian(a2.x, a2.y, woBase, randSampleBrdf);
            wi = mul(wi, preCalcData.toBaseLayerTangentSpace(surfaceDef.tangent));
            isDiffuseBounce = true;
            isDeltaDistribution = false;
        }
	} 

	//since the normal might not be the real geometry normal, could reflect ray inside object and assume reflection. Figure out better way to handle this later on.
	if(!allowTransmitted && (dot(surfaceDef.geometryNormal, wi) < 0))
	{
		wi = 0.f;
	}



#ifdef REJECT_SHADING_GEOMETRY_FRAME_MISMATCH
	if (allowTransmitted && sampleLayer == LAYERIND_SPEC_DIELECTRIC && !isZero(wi))
	{
		float3 woOS = mul(preCalcData.woBase, preCalcData.toBaseLayerTangentSpace(surfaceDef.tangent));
		bool transmittedGeometry = dot(woOS, surfaceDef.geometryNormal) * dot(wi, surfaceDef.geometryNormal) < 0;
		bool transmittedShading = preCalcData.woBase.y * dot(wi, preCalcData.baseLayerNormal) < 0;

		if (transmittedGeometry != transmittedShading)
		{
			wi = 0.f;
		}
	}
#endif

    isDiffuseBounceOut = isDiffuseBounce;
    isDeltaDistributionOut = isDeltaDistribution;
    
	return wi;
}

float compensateForShadingNormal(float absDotWoShading, float absDotWiShading, float absDotWoGeometry, float absDotWiGeometry)
{
    return (absDotWoShading * absDotWiGeometry) / (absDotWoGeometry * absDotWiShading);
}

SurfaceEvaluation evaluateSurface(in SurfaceSamplingContext surface, in float3 woObjSpace, in float3 wiObjSpace, in uint evaluateFlags)
{
	SurfaceDefinition surfaceDef = surface.surfaceDef;
	PrecalculatedSurfaceData precalculatedSurfData = surface.precalculatedSurfaceData;
	SurfaceEvaluation result;

    float2 a2 = precalculatedSurfData.a2;
    float2 a2CC = calculateRoughnessParams(surfaceDef.clearCoatRoughness, 0.f);

	float3 woCoating = precalculatedSurfData.woCoating;
	float3 woBase = precalculatedSurfData.woBase;

	bool exiting = precalculatedSurfData.exiting;
	float fromIOR = precalculatedSurfData.fromIOR;
	float toIOR = precalculatedSurfData.toIOR;
    
    float specOcclusion;
    float diffOcclusion;
    unpackOcclusion(surfaceDef.occlusionSpecDiffPacked, diffOcclusion, specOcclusion);

	SpectralSamples weightSum = (SpectralSamples)0.f;
	float pdfSum = 0.f;

    float3 wiCoating = mul(precalculatedSurfData.toCoatingLayerTangentSpace(surfaceDef.tangent), wiObjSpace);
    float3 wiBase = mul(precalculatedSurfData.toBaseLayerTangentSpace(surfaceDef.tangent), wiObjSpace);

	wiCoating = normalize(wiCoating);
	wiBase = normalize(wiBase);

    result.transmissionType = TRANSMISSION_TYPE_NONE;
    
    bool onlyPDF = (evaluateFlags &  EVALUATE_FLAGS_PDF_ONLY) != 0;
    bool lightPath = (evaluateFlags & EVALUATE_FLAGS_LIGHT_PATH) != 0;
    bool treatAsDelta = (evaluateFlags & EVALUATE_FLAGS_TREAT_AS_DELTA) != 0;
    bool negateSolidAngleProjection = (evaluateFlags & EVALUATE_FLAGS_DONT_PROJECT_SOLID_ANGLE) != 0;
	
    float dotWoGeometry = dot(woObjSpace, surfaceDef.geometryNormal);
    float dotWiGeometry = dot(wiObjSpace, surfaceDef.geometryNormal);
	
    bool baseReflected = dotWoGeometry * dotWiGeometry > 0;
    bool baseRefracted = dotWoGeometry * dotWiGeometry < 0;
	float energyLeft = 1.f;
    float energyLeftFromLayer = 1.f;
	
    float shadingNormalCompensationMultiplierCoating = lightPath ? compensateForShadingNormal(abs(woCoating.y), abs(wiCoating.y), abs(dotWoGeometry), abs(dotWiGeometry)) : 1.f;
    float shadingNormalCompensationMultiplierBase = lightPath ? compensateForShadingNormal(abs(woBase.y), abs(wiBase.y), abs(dotWoGeometry), abs(dotWiGeometry)) : 1.f;
    
    //append to normal compensation multiplier negation of solid angle 
    if (negateSolidAngleProjection)
    {
        shadingNormalCompensationMultiplierCoating = safeDiv(shadingNormalCompensationMultiplierCoating, abs(wiCoating.y));
        shadingNormalCompensationMultiplierBase = safeDiv(shadingNormalCompensationMultiplierBase, abs(wiBase.y));
    }
    
    float dielAmount = 1.f - surfaceDef.metalness;
    float specAmount = dielAmount * surfaceDef.specularAmount;
    
    bool applyInference = surfaceDef.thinFilmThicknessNM > 0 && surfaceDef.clearCoatAmount > 0 && (surfaceDef.metalness > 0 || specAmount > 0);
    applyInference = applyInference && (isDeltaGGX(a2CC) == isDeltaGGX(a2));
    float4 coatingAmplitude = 0;
	//coating layer (ggx & sheen)
	{
        if (surfaceDef.clearCoatAmount > 0)
        {
           
            float etaR = surfaceDef.clearCoatIOR / fromIOR;
            ReflectionDielectric coating = ReflectionDielectric::init(a2CC, surfaceDef.clearCoatRoughness, etaR);

            if (isDeltaGGX(a2CC) == treatAsDelta)
            {
                float pdf = coating.pdf(woCoating, wiCoating);
                bool coatingReflected = dot(woObjSpace, surfaceDef.geometryNormal) * dot(wiObjSpace, surfaceDef.geometryNormal) > 0;
                if (pdf > 0 && coatingReflected)
                {
                    if (!onlyPDF)
                    {
                        SpectralSamples weight = evaluateLayer(coating, woCoating, wiCoating, surfaceDef.clearCoatAmount * energyLeft) * shadingNormalCompensationMultiplierCoating * specOcclusion;

                        if (applyInference)
                        {
                            coatingAmplitude = weight.toFloat4();
                        }
                        else
                        {
                            weightSum = weightSum + weight;
                        }
                        
                    }
                    if(isDeltaGGX(a2CC) == treatAsDelta)
                    {
                        pdfSum += surface.samplingProbabilities[LAYERIND_COATING_GGX] * pdf;
                    }
                    
                }
            }
            
            energyLeftFromLayer *= coating.getEnergyLeftAfterLayer(woCoating, wiCoating, surfaceDef.clearCoatAmount);
            
        }

        if (surfaceDef.sheenAmount > 0)
        {
            ReflectionSheen sheen = ReflectionSheen::init(surfaceDef.sheenColor, surfaceDef.sheenRoughness);
            if (isDeltaSheen(surfaceDef.sheenRoughness) == treatAsDelta)
            {
                float pdf = sheen.pdf(woBase, wiBase);
                if (pdf > 0 && baseReflected)
                {
                    if (!onlyPDF)
                    {
                        weightSum = weightSum + evaluateLayer(sheen, woBase, wiBase, surfaceDef.sheenAmount * energyLeft) * shadingNormalCompensationMultiplierBase * specOcclusion;
                        
                    }
                    pdfSum += surface.samplingProbabilities[LAYERIND_COATING_SHEEN] * pdf;
                }
            }
            
            energyLeftFromLayer *= sheen.getEnergyLeftAfterLayer(woBase, wiBase, surfaceDef.sheenAmount);
        }
    }
	
	//reduce energy from coating
    energyLeft *= energyLeftFromLayer;
    energyLeftFromLayer = 1.f;
	
	//spec layer dielelectric & conductor
	{
        if (surfaceDef.metalness > 0)
        {
            ReflectionConductor conductor = ReflectionConductor::init(surfaceDef.albedo, surfaceDef.specular, a2, surfaceDef.roughness, fromIOR);
            
            if (isDeltaGGX(a2) == treatAsDelta)
            {
                float pdf = conductor.pdf(woBase, wiBase);

                if (pdf > 0 && baseReflected)
                {
                    if (!onlyPDF)
                    {
                        SpectralSamples weight = evaluateLayer(conductor, woBase, wiBase, surfaceDef.metalness * energyLeft) * shadingNormalCompensationMultiplierBase * specOcclusion;
                        if (applyInference)
                        {
                            float etaRCoating = surfaceDef.clearCoatIOR / fromIOR;
                            float3 wm = normalize(woCoating + wiCoating);
                            float4 w = weight.toFloat4();
                            float phaseShift = etaRCoating > 1 ? 0.5f : 0.f; //with metal, always assuming that the substrate (metal) is denser than film
                            float4 m = applyThinFilmInference(getSpectralSampleLambdas(), coatingAmplitude, w, surfaceDef.clearCoatIOR, etaRCoating, phaseShift, dot(woCoating, wm), surfaceDef.thinFilmThicknessNM);
                            weight.fromFloat4(m);

                        }
                        
                        weightSum = weightSum + weight;
                    }
                    if (isDeltaGGX(a2) == treatAsDelta)
                    {
                        pdfSum += surface.samplingProbabilities[LAYERIND_SPEC_CONDUCTOR] * pdf;
                    }
                    
                }
            }
            energyLeftFromLayer *= conductor.getEnergyLeftAfterLayer(woBase, wiBase, surfaceDef.metalness);
        }

        
        if (specAmount > 0 && baseReflected) //spec reflection
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
            SpecularLayer specLayer = SpecularLayer::init(a2, surfaceDef.roughness, etaR, surfaceDef.specularAmount > 0, surfaceDef.transparency > 0);
            
            
            if (isDeltaGGX(a2) == treatAsDelta)
            {
                float pdf = specLayer.pdf(woBase, wiBase);
                if (pdf > 0)
                {
                    if (!onlyPDF)
                    {
                        SpectralSamples weight = evaluateLayer(specLayer, woBase, wiBase, specAmount * energyLeft) * shadingNormalCompensationMultiplierBase * specOcclusion;
                        if (applyInference)
                        {
                            float etaRCoating = surfaceDef.clearCoatIOR / fromIOR;
                            float3 wm = normalize(woCoating + wiCoating);
                            float4 w = weight.toFloat4();
                            float phaseShift = (etaRCoating > 1 && toIOR < surfaceDef.clearCoatIOR) ? 0.5f : 0.f;
                            float4 m = applyThinFilmInference(getSpectralSampleLambdas(), coatingAmplitude, w, surfaceDef.clearCoatIOR, etaRCoating, phaseShift, dot(woCoating, wm), surfaceDef.thinFilmThicknessNM);
                            weight.fromFloat4(m);

                        }
                        weightSum = weightSum + weight;
                        
                    }
                    
                    if (isDeltaGGX(a2) == treatAsDelta)
                    {
                        pdfSum += surface.samplingProbabilities[LAYERIND_SPEC_DIELECTRIC] * pdf;
                    }
                    
                }
            }
            energyLeftFromLayer *= specLayer.getEnergyLeftAfterLayer(woBase, wiBase, specAmount);
        }
    }
	
    energyLeft *= energyLeftFromLayer;
    energyLeftFromLayer = 1.f;

	//bottom layer (diffuse and transmittance)
	{
        //the subsurface share of the diffuse layer is handled by the subsurface event, not here
        float diffuseAmount = (1.f - surfaceDef.transparency) * dielAmount * (1.f - surfaceDef.subsurfaceAmount);
        if (diffuseAmount > 0 && !treatAsDelta)
        {
            DiffuseLayer diff = DiffuseLayer::init(surfaceDef.albedo, a2);
            float pdf = diff.pdf(woBase, wiBase);

            if (pdf > 0 && baseReflected)
            {
                if (!onlyPDF)
                {
                    
                    weightSum = weightSum + evaluateLayer(diff, woBase, wiBase, diffuseAmount * energyLeft) * shadingNormalCompensationMultiplierBase * diffOcclusion;
                    energyLeftFromLayer *= diff.getEnergyLeftAfterLayer(woBase, wiBase, diffuseAmount);
                }
                pdfSum += surface.samplingProbabilities[LAYERIND_DIFFUSE_REFL] * pdf;
            }

        }

        float transparencyAmount = surfaceDef.transparency * dielAmount;
        if (transparencyAmount > 0 && baseRefracted && (isDeltaGGX(a2) == treatAsDelta)) //spec refraction
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
                result.transmissionType |= TRANSMISSION_TYPE_DISPERSED;
            }
        
            float etaR = toIOR / fromIOR;
            SpecularLayer specLayer = SpecularLayer::init(a2, surfaceDef.roughness, etaR, surfaceDef.specularAmount > 0, surfaceDef.transparency > 0);
            float pdf = specLayer.pdf(woBase, wiBase);
            if (pdf > 0 )
            {
                pdfSum += surface.samplingProbabilities[LAYERIND_SPEC_DIELECTRIC] * pdf;

                bool twoSided = isSurfaceTwoSided(surfaceDef.flags);
                bool wasTransmitted = !twoSided;

                float solidAngleCompression = 1.f;

			
                if (wasTransmitted)
                {
				    //if transmitted, handle solid angle compression (btdf asymmetry), but only for camera path
                    if (!lightPath)
                    {
                        solidAngleCompression *= sqr(1.f / etaR);
                    }
				    
                    result.transmissionType |= exiting ? TRANSMISSION_TYPE_EXITED : TRANSMISSION_TYPE_ENTERED;

                }
                else if (twoSided)
                {
                    result.transmissionType |= TRANSMISSION_TYPE_ENTERED | TRANSMISSION_TYPE_EXITED;

                }

                
                if (!onlyPDF)
                {
                    weightSum = weightSum + evaluateLayer(specLayer, woBase, wiBase, transparencyAmount * energyLeft) * solidAngleCompression * shadingNormalCompensationMultiplierBase * specOcclusion;
                }
            }
        }
    }
    
	result.pdf = pdfSum;
	result.weight = weightSum;
	return result;
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
    surfaceDataOut.coatingLayerNormal = surfaceDef.coatingLayerNormal;
    
#ifdef NUDGE_SHADING_NORMALS
    bool twoSided = isSurfaceTwoSided(surfaceDef.flags);
    surfaceDataOut.baseLayerNormal = nudgeNormal(-woObjSpace, surfaceDataOut.baseLayerNormal, surfaceDef.roughness, surfaceDef.transparency, twoSided);
    surfaceDataOut.coatingLayerNormal = nudgeNormal(-woObjSpace, surfaceDataOut.coatingLayerNormal, surfaceDef.clearCoatRoughness, surfaceDef.transparency, twoSided);
#endif

    float3 woCoating = mul(surfaceDataOut.toCoatingLayerTangentSpace(surfaceDef.tangent), woObjSpace);
    float3 woBase = mul(surfaceDataOut.toBaseLayerTangentSpace(surfaceDef.tangent), woObjSpace);
    woCoating = normalize(woCoating);
    woBase = normalize(woBase);
    surfaceDataOut.woBase = woBase;
    surfaceDataOut.woCoating = woCoating;


}

SurfaceSamplingContext makeSurfaceSamplingContext(in SurfaceDefinition surfaceDef, in float currentIOR, in float previousIOR, in float3 woObjSpace, in bool triangleHitFrontFace)
{
    SurfaceSamplingContext surface;
    surface.surfaceDef = surfaceDef;
    getPrecalculatedSurfaceData(surfaceDef, currentIOR, previousIOR, woObjSpace, triangleHitFrontFace, surface.precalculatedSurfaceData);
    calculateNormalizedMaterialLayerSamplingProbabilities(surfaceDef, surface.precalculatedSurfaceData, surface.samplingProbabilities);
    return surface;
}


#endif