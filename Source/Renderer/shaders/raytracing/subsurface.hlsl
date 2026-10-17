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
	SpectralSamples weight; //Rd(|po - pi|) / (pdfPosition * probe hit selection probability)
	float pdfPosition; //area density of the exit point, without the probe hit selection (see pdfSubsurfaceExit)
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


/*
Christensen-Burley profile, normalized so that it integrates to 1 over the plane: R(r) = (e^(-r/d) + e^(-r/(3d))) / (8 pi d r)
Rd = albedo * R, with the radius treated as the diffuse mean free path and d = radius / s(albedo) (searchlight fit of the paper)
*/
#define SUBSURFACE_SPECTRAL_LANES 4
#define SUBSURFACE_MAX_PROBE_HITS 8
#define SUBSURFACE_MIN_D 1e-6f
#define SUBSURFACE_RMAX_FACTOR 20.7232658f //3 * ln(1000), the 0.999 tail of the slow component

float burleyScale(float albedo)
{
	float x = abs(albedo - 0.8f);
	return 1.85f - albedo + 7.f * x * x * x;
}

float burleyD(float radius, float albedo)
{
	return max(radius, SUBSURFACE_MIN_D) / burleyScale(albedo);
}

float burleyRMax(float d)
{
	return SUBSURFACE_RMAX_FACTOR * d;
}

//area density over the plane
float burleyProfileArea(float r, float d)
{
	r = max(r, d * 1e-4f);
	return (exp(-r / d) + exp(-r / (3.f * d))) / (8.f * PI * d * r);
}

//the radial density is a mixture of exponentials, (1/4) e^(-r/d)/d + (3/4) e^(-r/(3d))/(3d)
float burleySampleRadius(float u, float d)
{
	float scale = d;
	if (u < 0.25f)
	{
		u *= 4.f;
	}
	else
	{
		u = (u - 0.25f) / 0.75f;
		scale = 3.f * d;
	}
	return -scale * log(max(1.f - u, 1e-7f));
}

//Rd(distance) for all the spectral lanes
float4 evaluateSubsurfaceRd(in SurfaceDefinition surfaceDef, float distance)
{
	float4 rd;
	for (uint lane = 0; lane < SUBSURFACE_SPECTRAL_LANES; ++lane)
	{
		float albedo = surfaceDef.albedo[lane];
		rd[lane] = albedo * burleyProfileArea(distance, burleyD(surfaceDef.subsurfaceRadius[lane], albedo));
	}
	return rd;
}

//probe axis k: 0 = normal (p = 0.5), 1 = tangent, 2 = bitangent (p = 0.25 each). The frame only depends on the normal so that the pdf can be evaluated from positions and normals alone
void getSubsurfaceProbeFrame(in float3 normalWS, in uint axisIndex, out float3 axisOut, out float3 planeU, out float3 planeV)
{
	float3 t;
	float3 b;
	constructVectorBase(normalWS, t, b);
	if (axisIndex == 0)
	{
		axisOut = normalWS;
		planeU = t;
		planeV = b;
	}
	else if (axisIndex == 1)
	{
		axisOut = t;
		planeU = b;
		planeV = normalWS;
	}
	else
	{
		axisOut = b;
		planeU = normalWS;
		planeV = t;
	}
}

float getSubsurfaceProbeAxisProbability(uint axisIndex)
{
	return axisIndex == 0 ? 0.5f : 0.25f;
}

//area density of the exit point (to) from the entry point (from), combined over all the probe axes and spectral lanes (one sample MIS).
//The selection between the probe hits is not included (it can't be reproduced without retracing the probe), it only affects the sample weight
float pdfSubsurfaceExit(in SurfaceDefinition surfaceDef, in float3 fromPosWS, in float3 fromNormalWS, in float3 toPosWS, in float3 toNormalWS)
{
	float3 d = toPosWS - fromPosWS;
	float pdf = 0.f;
	for (uint axisIndex = 0; axisIndex < 3; ++axisIndex)
	{
		float3 axis;
		float3 planeU;
		float3 planeV;
		getSubsurfaceProbeFrame(fromNormalWS, axisIndex, axis, planeU, planeV);

		float rProj = length(float2(dot(d, planeU), dot(d, planeV)));
		float axisPdf = 0.f;
		for (uint lane = 0; lane < SUBSURFACE_SPECTRAL_LANES; ++lane)
		{
			float dLane = burleyD(surfaceDef.subsurfaceRadius[lane], surfaceDef.albedo[lane]);
			if (rProj < burleyRMax(dLane))
			{
				axisPdf += burleyProfileArea(rProj, dLane);
			}
		}
		pdf += getSubsurfaceProbeAxisProbability(axisIndex) * (1.f / SUBSURFACE_SPECTRAL_LANES) * axisPdf * abs(dot(toNormalWS, axis));
	}
	return pdf;
}

//probe ray sampling (King et al. 2013): pick an axis and a spectral lane, sample a point on the disk orthogonal to the axis and project it to the surface of the same instance along the axis
bool sampleSubsurfaceExit(in SurfaceDefinition surfaceDef, in SurfacePoint entryPoint, in float3 rand, out SubsurfaceExitSample exitOut)
{
	exitOut = (SubsurfaceExitSample)0;

	//rand.x selects the axis and the lane, the rest of it selects between the probe hits
	float u = rand.x;
	uint axisIndex;
	if (u < 0.5f)
	{
		axisIndex = 0;
		u *= 2.f;
	}
	else if (u < 0.75f)
	{
		axisIndex = 1;
		u = (u - 0.5f) * 4.f;
	}
	else
	{
		axisIndex = 2;
		u = (u - 0.75f) * 4.f;
	}
	uint lane = min(uint(u * SUBSURFACE_SPECTRAL_LANES), SUBSURFACE_SPECTRAL_LANES - 1);
	float uSelect = saturate(u * SUBSURFACE_SPECTRAL_LANES - lane);

	float dLane = burleyD(surfaceDef.subsurfaceRadius[lane], surfaceDef.albedo[lane]);
	float rMax = burleyRMax(dLane);
	float r = burleySampleRadius(rand.y, dLane);
	if (r >= rMax)
	{
		return false;
	}

	float3 axis;
	float3 planeU;
	float3 planeV;
	getSubsurfaceProbeFrame(entryPoint.geometryNormalWS, axisIndex, axis, planeU, planeV);

	float sinPhi;
	float cosPhi;
	sincos(2.f * PI * rand.z, sinPhi, cosPhi);

	//chord of the r_max sphere through the disk point. Keep a minimum length, a tiny profile would otherwise lose the hits to float precision
	float3 entryPos = entryPoint.positionWS;
	float maxCoord = max(max(abs(entryPos.x), abs(entryPos.y)), abs(entryPos.z));
	float halfLength = max(sqrt(rMax * rMax - r * r), 1e-3f + 1e-5f * maxCoord);
	float3 probeOrigin = entryPos + r * (cosPhi * planeU + sinPhi * planeV) - axis * halfLength;

	float selectionWeightSum = 0.f;
	float selectedSelectionWeight = 0.f;
	float4 selectedRd = 0;
	float selectedT = 0.f;
	uint selectedPrimitive = 0;
	float2 selectedBarycentrics = 0;
	bool selectedFrontFace = false;
	float tMin = 0.f;
	for (uint i = 0; i < SUBSURFACE_MAX_PROBE_HITS; ++i)
	{
		RayQuery<RAY_FLAG_FORCE_NON_OPAQUE> q;

		RayDesc ray;
		ray.Origin = probeOrigin;
		ray.Direction = axis;
		ray.TMin = tMin;
		ray.TMax = 2.f * halfLength;

		q.TraceRayInline(g_accelerationStructure, RAY_FLAG_NONE, ~0, ray);

		//closest hit on the same instance, everything else is ignored
		while (q.Proceed())
		{
			if (q.CandidateType() == CANDIDATE_NON_OPAQUE_TRIANGLE && q.CandidateInstanceID() == entryPoint.instanceIndex && alphaTestPasses(q.CandidateInstanceID(), q.CandidatePrimitiveIndex(), q.CandidateTriangleBarycentrics()))
			{
				q.CommitNonOpaqueTriangleHit();
			}
		}

		if (q.CommittedStatus() != COMMITTED_TRIANGLE_HIT)
		{
			break;
		}

		//select between the hits proportionally to the profile without storing them (keep the k:th hit with probability w_k / (w_1 + ... + w_k)).
		//Far away hits contribute little, so this has less variance than a uniform selection on thin and folded geometry
		float4 rd = evaluateSubsurfaceRd(surfaceDef, length(probeOrigin + axis * q.CommittedRayT() - entryPos));
		float selectionWeight = rd.x + rd.y + rd.z + rd.w;
		selectionWeightSum += selectionWeight;
		float keepProbability = selectionWeightSum > 0.f ? selectionWeight / selectionWeightSum : 0.f;
		//keepProbability is 1 for the first hit with a nonzero weight, the selection random number is then left as is
		if (keepProbability >= 1.f || uSelect < keepProbability)
		{
			uSelect = keepProbability >= 1.f ? uSelect : uSelect / keepProbability;
			selectedSelectionWeight = selectionWeight;
			selectedRd = rd;
			selectedT = q.CommittedRayT();
			selectedPrimitive = q.CommittedPrimitiveIndex();
			selectedBarycentrics = q.CommittedTriangleBarycentrics();
			selectedFrontFace = q.CommittedTriangleFrontFace();
		}
		else
		{
			uSelect = (uSelect - keepProbability) / (1.f - keepProbability);
		}

		tMin = q.CommittedRayT() + DEFAULT_RAY_MIN_T;
	}

	//no hits, or all of them so far that the profile is zero
	if (!(selectedSelectionWeight > 0.f))
	{
		return false;
	}

	uint2 matMeshIndices = getMaterialAndMeshIndicesForInstance(entryPoint.instanceIndex);
	MeshEntryGPU meshEntry = getMeshEntry(matMeshIndices.y);
	uint3 indices = fetchIndices(meshEntry.indexBuffer, selectedPrimitive);
	float3 normalOS = fetchMeshTriangleNormal(meshEntry.positionBuffer, indices, 0);
	float3x3 worldToObj = (float3x3)getTransformDataForInstance(entryPoint.instanceIndex).getWorldToObj();
	float3 normalWS = normalize(mul(normalOS, worldToObj));

	//orient by the side the probe hit (front face faces against the probe), this doesn't depend on the winding or mirroring transforms
	float3 frontNormalWS = ((dot(normalWS, axis) < 0) == selectedFrontFace) ? normalWS : -normalWS;

	SurfacePoint exitPoint;
	exitPoint.positionWS = probeOrigin + axis * selectedT;
	exitPoint.instanceIndex = entryPoint.instanceIndex;
	//the path entered from the outside of the entry point, leave to the same side at the exit
	exitPoint.geometryNormalWS = entryPoint.frontFace ? frontNormalWS : -frontNormalWS;
	exitPoint.primitiveIndex = selectedPrimitive;
	exitPoint.barycentrics = selectedBarycentrics;
	exitPoint.frontFace = entryPoint.frontFace;

	float pdf = pdfSubsurfaceExit(surfaceDef, entryPos, entryPoint.geometryNormalWS, exitPoint.positionWS, exitPoint.geometryNormalWS);
	if (!(pdf > 0.f))
	{
		return false;
	}

	//the profile is evaluated at the actual distance, the projected distances are only used for the pdf. Divide by the hit selection probability too
	float selectionProbability = selectedSelectionWeight / selectionWeightSum;
	exitOut.weight.fromFloat4(selectedRd / (pdf * selectionProbability));
	exitOut.exitPoint = exitPoint;
	exitOut.pdfPosition = pdf;
	return true;
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
