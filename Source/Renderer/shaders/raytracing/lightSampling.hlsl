#ifndef RAYTRACE_LIGHT_SAMPLING_INCL
#define RAYTRACE_LIGHT_SAMPLING_INCL

#include "../common/miscBrdf.hlsl"

struct LightSampleOutput
{
    SpectralSamples radiance;
    float3 positionWS;
    float3 directionWS;
    float3 normalWS;
    float pdfPos;
    float pdfDir;
    float emissionFocus;
    float emissionProfileWeight;
    uint instanceIndex;
    uint primitiveIndex;
    float2 baryCentrics;
    bool sampledAsTwoSided;
};

//chance of starting a BDPT light subpath from the environment when the scene has lights
#define ENV_LIGHT_SELECTION_PROBABILITY 0.0f

float environmentSelectionPDF()
{
    return g_lightCount == 0 ? 1.f : ENV_LIGHT_SELECTION_PROBABILITY;
}

//light selection uses an alias table over all lights, built on the CPU from estimated light power
uint sampleLightIndex(float u)
{
    float scaled = u * g_lightCount;
    uint index = min((uint) scaled, g_lightCount - 1);
    LightEntryGPU entry = g_lights[index];
    return (scaled - index) < entry.aliasThreshold ? index : entry.aliasIndex;
}

float lightSelectionPDF(uint lightIndex)
{
    return g_lights[lightIndex].selectionPDF;
}

//separate alias table over the punctual lights only, used when they are the only explicitly sampled lights
uint samplePunctualLightIndex(float u)
{
    uint firstPunctual = g_lightCount - g_punctualLightCount;
    float scaled = u * g_punctualLightCount;
    uint index = min((uint) scaled, g_punctualLightCount - 1);
    LightEntryGPU entry = g_lights[firstPunctual + index];
    return firstPunctual + ((scaled - index) < entry.punctualAliasThreshold ? index : entry.punctualAliasIndex);
}

float punctualLightSelectionPDF(uint lightIndex)
{
    return g_lights[lightIndex].punctualSelectionPDF;
}

//selection probability of the mesh light a ray hit, 0 for instances that are not lights
float meshLightSelectionPDFForInstance(uint instanceIndex)
{
    return asfloat(g_instanceLightSelectionPDFs.Load(instanceIndex << 2));
}

//directions are sampled uniformly in spherical coordinates around +z
float pdfEnvironmentDirection(float3 dir)
{
    float cosTheta = clamp(dir.z, -1.f, 1.f);
    float sinTheta = sqrt(max(0.f, 1.f - cosTheta * cosTheta));
    return safeDiv(1.f, 2.f * PI * PI * sinTheta);
}

float pdfEnvironmentPosition()
{
    return safeDiv(1.f, PI * getWorldCenterAndRadiusSqr().w);
}

void sampleEnvironmentLighting(float4 randValues, out SpectralSamples radianceOut, out float3 posOut, out float3 dirOut, out float pdfPosOut, out float pdfDirOut)
{
	//sample direction
    float pdfDir;
    float3 lightDir;
	{
        float2 uv = randValues.xy;
        float theta = uv.x * PI;
        float phi = uv.y * 2 * PI;
        float cosTheta;
        float sinTheta;
        float cosPhi;
        float sinPhi;

        sincos(theta, sinTheta, cosTheta);
        sincos(phi, sinPhi, cosPhi);

        lightDir = float3(sinTheta * cosPhi, sinTheta * sinPhi, cosTheta);
        pdfDir = safeDiv(1.f, (2.f * PI * PI * sinTheta));
    }

	//sample position
    float pdfPos;
    float3 lightPos;
	{
        float4 worldCenterRadSqr = getWorldCenterAndRadiusSqr();
        float3 v1, v2;
        constructVectorBase(lightDir, v1, v2);
        float2 cd = sampleConcentricDisk(randValues.zw);
        float radius = sqrt(worldCenterRadSqr.w);
        //disk perpendicular to the direction, placed behind the world bounds so the rays cross the whole scene
        lightPos = worldCenterRadSqr.xyz - lightDir * radius + radius * (cd.x * v1 + cd.y * v2);
        pdfPos = safeDiv(1.f, PI * worldCenterRadSqr.w);
    }

    radianceOut.setFromRGBUnbounded(getSkyBoxColor(-lightDir, g_envType, g_envTexIndex).xyz * g_envIntensityScale);
    posOut = lightPos;
    dirOut = lightDir;
    pdfPosOut = pdfPos;
    pdfDirOut = pdfDir;

}


void sampleLight(uint lightIndex, float3 randValuesPos, float2 randValuesDir, out LightSampleOutput output)
{
    LightEntryGPU lightEntry = g_lights[lightIndex];
    MeshEntryGPU meshEntry = getMeshEntry(lightEntry.meshIndex);

    uint primCount = (meshEntry.indexCount / 3);
    uint primitiveIndex = min(primCount * randValuesPos.z, primCount - 1);

    float2 randBary = randValuesPos.xy;
    if(randBary.x + randBary.y > 1)
    {
        randBary.x = 1.f - randBary.x;
        randBary.y = 1.f - randBary.y;
    }
    
    float3 barycentrics = float3(1 - randBary.x - randBary.y, randBary.x, randBary.y);
    uint3 indices = fetchIndices(meshEntry.indexBuffer, primitiveIndex);

    float3 p1, p2, p3;
    fetchMeshPositions(meshEntry.positionBuffer, indices, p1, p2, p3);
    p1 = mul(lightEntry.transform, float4(p1, 1)).xyz;
    p2 = mul(lightEntry.transform, float4(p2, 1)).xyz;
    p3 = mul(lightEntry.transform, float4(p3, 1)).xyz;
    float3 geometryNormalWS = cross(p2 - p1, p3 - p1);
    float geomNormalLength = length(geometryNormalWS);
    geometryNormalWS /= geomNormalLength;
    
    float area = geomNormalLength * 0.5f;
    float3 pos = barycentrics.x * p1 + barycentrics.y * p2 + barycentrics.z * p3;
    
    float3 tangent = meshHasValidTangents(meshEntry.tangentBuffer) ? fetchMeshTangent(meshEntry.tangentBuffer, indices, barycentrics) : float3(1.f, 0.f, 0.f);
    float2 uv = meshHasValidUVs(meshEntry.uvBuffer) ? fetchMeshUV(meshEntry.uvBuffer, indices, barycentrics) : float2(0.5f, 0.5f);

    MaterialEntryGPU matEntry = getMaterialEntry(lightEntry.matIndex);
    SurfaceDefinitionRGB surfaceDefRGB;
    fetchSurfaceMaterialParameters(matEntry, surfaceDefRGB, geometryNormalWS, geometryNormalWS, geometryNormalWS, tangent);
    modifySurfaceEmissionWithTexture(matEntry, uv, surfaceDefRGB);
    
    tangent = normalize(mul((float3x3)lightEntry.transformInvTransp, tangent));
    bool twoSided = isSurfaceTwoSided(surfaceDefRGB.flags);
    float emissionFocus = surfaceDefRGB.emissionFocus;
    float3 lightDir;
    if(twoSided)
    {
        lightDir = sampleCosinePowerWeightedSphere(randValuesDir, emissionFocus);
    }
    else
    {
        lightDir = sampleCosinePowerWeightedHemisphere(randValuesDir, emissionFocus);
    }
    float3x3 tanToWS = constructBasisTransform(geometryNormalWS, tangent);

	//radiance is not weighted with emission profile weight since the light might be evaluated for different direction than the one sampled here (if only pos is relevant)
    output.radiance.setFromRGBUnbounded(surfaceDefRGB.emissive);
    output.pdfPos = 1.f / (primCount * area);
    output.pdfDir = twoSided ? pdfCosinePowerWeightedSphere(lightDir.y, emissionFocus) : pdfCosinePowerWeightedHemisphere(lightDir.y, emissionFocus);
    output.emissionFocus = emissionFocus;
    output.emissionProfileWeight = evaluateEmissionProfile(lightDir.y, emissionFocus);
    output.positionWS = pos;
    output.directionWS = mul(lightDir, tanToWS);
    output.normalWS = geometryNormalWS;
    output.instanceIndex = lightEntry.instanceIndex;
    output.primitiveIndex = primitiveIndex;
    output.baryCentrics = barycentrics.yz;
    output.sampledAsTwoSided = twoSided;

}

//pdfs of a BDPT light subpath starting from the given point on a mesh light
void pdfForSamplingLight(uint instanceIndex, uint primitiveIndex, float2 bary, float3 lightSurfaceNormal, float3 towardsDir, out float lightPickPDF, out float posPDF, out float dirPDF)
{
    
    lightPickPDF = (1.f - environmentSelectionPDF()) * meshLightSelectionPDFForInstance(instanceIndex);
    
    uint2 matMeshIndices = getMaterialAndMeshIndicesForInstance(instanceIndex);
    RenderObjectTransformDataGPU transformData = getTransformDataForInstance(instanceIndex);
    MeshEntryGPU meshEntry = getMeshEntry(matMeshIndices.y);
    MaterialEntryGPU lightMatEntry = getMaterialEntry(matMeshIndices.x);
    bool twoSided = (lightMatEntry.materialMask & MaterialMask_TwoSided) != 0;
    uint primCount = (meshEntry.indexCount / 3);

    float3 barycentrics = float3(1 - bary.x - bary.y, bary.x, bary.y);
    uint3 indices = fetchIndices(meshEntry.indexBuffer, primitiveIndex);
    float3x4 transf = transformData.getObjToWorld();
    
    float3 p1, p2, p3;
    fetchMeshPositions(meshEntry.positionBuffer, indices, p1, p2, p3);
    p1 = mul(transf, float4(p1, 1)).xyz;
    p2 = mul(transf, float4(p2, 1)).xyz;
    p3 = mul(transf, float4(p3, 1)).xyz;
    float area = length(cross(p2 - p1, p3 - p1)) * 0.5f;
    
    posPDF = 1.f / (primCount * area);

    float cosTheta = dot(lightSurfaceNormal, towardsDir);
    if (twoSided)
    {
        dirPDF = pdfCosinePowerWeightedSphere(cosTheta, lightMatEntry.emissionFocus);
    }
    else
    {
        dirPDF = cosTheta >= 0 ? pdfCosinePowerWeightedHemisphere(cosTheta, lightMatEntry.emissionFocus) : 0;
    }

}



void pdfForSamplingEnv(float3 towardsDir, out float lightPickPDF, out float posPDF, out float dirPDF)
{
    lightPickPDF = environmentSelectionPDF();
    dirPDF = pdfEnvironmentDirection(towardsDir);
    posPDF = pdfEnvironmentPosition();
}


bool isPunctualLight(LightEntryGPU light)
{
    return light.lightType != LIGHT_TYPE_MESH;
}

float spotFalloff(float cosTheta, float cosInner, float cosOuter)
{
    if (cosInner <= cosOuter)
    {
        return cosTheta >= cosOuter ? 1.f : 0.f;
    }
    return smoothstep(cosOuter, cosInner, cosTheta);
}

//relative emitted intensity towards dirFromLightWS
float punctualLightAngularFalloff(LightEntryGPU light, float3 dirFromLightWS)
{
    if (light.lightType == LIGHT_TYPE_SPOT)
    {
        return spotFalloff(dot(dirFromLightWS, light.directionCosOuter.xyz), light.positionCosInner.w, light.directionCosOuter.w);
    }
    return 1.f;
}

struct PunctualLightIncidence
{
    float3 toLightWS;
    float distance; //DEFAULT_RAY_MAX_T for directional lights
    float3 intensity; //incident at the reference point: falloff and inverse square distance included
};

PunctualLightIncidence evaluatePunctualLightIncidence(LightEntryGPU light, float3 refPosWS)
{
    PunctualLightIncidence result;
    if (light.lightType == LIGHT_TYPE_DIRECTIONAL)
    {
        result.toLightWS = -light.directionCosOuter.xyz;
        result.distance = DEFAULT_RAY_MAX_T;
        result.intensity = light.intensity.xyz;
    }
    else
    {
        float3 toLight = light.positionCosInner.xyz - refPosWS;
        float distSqr = max(dot(toLight, toLight), 1e-10f);
        result.distance = sqrt(distSqr);
        result.toLightWS = toLight / result.distance;
        result.intensity = light.intensity.xyz * (punctualLightAngularFalloff(light, -result.toLightWS) / distSqr);
    }
    return result;
}

//directional lights emit from a disk perpendicular to the light direction that covers the world bounds
float pdfDirectionalLightPosition()
{
    return safeDiv(1.f, PI * getWorldCenterAndRadiusSqr().w);
}

//density of emitting towards dirFromLightWS. Directional lights are delta in direction, 1 is returned for them
float pdfPunctualLightEmissionDirection(LightEntryGPU light, float3 dirFromLightWS)
{
    if (light.lightType == LIGHT_TYPE_SPOT)
    {
        return dot(dirFromLightWS, light.directionCosOuter.xyz) >= light.directionCosOuter.w ? pdfUniformCone(light.directionCosOuter.w) : 0.f;
    }
    else if (light.lightType == LIGHT_TYPE_POINT)
    {
        return pdfSphere();
    }
    return 1.f;
}

//samples an emission ray from a punctual light. Position is delta for point and spot lights (pdfPos 1), direction for directional lights (pdfDir 1).
//The normal is set to the emitted direction so that the cosine at the light is 1
void samplePunctualLightEmission(uint lightIndex, float4 randValues, out LightSampleOutput output)
{
    LightEntryGPU light = g_lights[lightIndex];
    output = (LightSampleOutput) 0;
    output.radiance.setFromRGBUnbounded(light.intensity.xyz);
    output.instanceIndex = -1;
    output.primitiveIndex = lightIndex;
    output.baryCentrics = 0;
    output.emissionFocus = 0;
    output.sampledAsTwoSided = true;

    if (light.lightType == LIGHT_TYPE_DIRECTIONAL)
    {
        float3 dir = light.directionCosOuter.xyz;
        float4 worldCenterRadSqr = getWorldCenterAndRadiusSqr();
        float radius = sqrt(worldCenterRadSqr.w);
        float3 v1, v2;
        constructVectorBase(dir, v1, v2);
        float2 cd = sampleConcentricDisk(randValues.xy);
        output.positionWS = worldCenterRadSqr.xyz - dir * radius + radius * (cd.x * v1 + cd.y * v2);
        output.directionWS = dir;
        output.pdfPos = pdfDirectionalLightPosition();
        output.pdfDir = 1.f;
        output.emissionProfileWeight = 1.f;
    }
    else
    {
        output.positionWS = light.positionCosInner.xyz;
        output.pdfPos = 1.f;
        if (light.lightType == LIGHT_TYPE_SPOT)
        {
            float3 axis = light.directionCosOuter.xyz;
            float3 v1, v2;
            constructVectorBase(axis, v1, v2);
            float3 local = sampleUniformCone(randValues.xy, light.directionCosOuter.w);
            output.directionWS = normalize(local.x * v1 + local.y * axis + local.z * v2);
            output.pdfDir = pdfUniformCone(light.directionCosOuter.w);
            output.emissionProfileWeight = punctualLightAngularFalloff(light, output.directionWS);
        }
        else
        {
            output.directionWS = sampleSphere(randValues.xy);
            output.pdfDir = pdfSphere();
            output.emissionProfileWeight = 1.f;
        }
    }
    output.normalWS = output.directionWS;
}

float solidAngleToAreaDensityMultiplier(float3 fromToUnnormalized, float3 toNormal)
{
    float invDistSqr = 1.f / dot(fromToUnnormalized, fromToUnnormalized);
    float absDot = abs(dot(toNormal, fromToUnnormalized * sqrt(invDistSqr)));
    return absDot * invDistSqr;
}

float areaDensityToSolidAngleMultiplier(float3 fromToUnnormalized, float3 toNormal)
{
    return 1.f / (solidAngleToAreaDensityMultiplier(fromToUnnormalized, toNormal));
}





#endif