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
    uint instanceIndex;
    uint primitiveIndex;
    float2 baryCentrics;
    bool sampledAsTwoSided;
};

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
        lightPos = worldCenterRadSqr.xyz + sqrt(worldCenterRadSqr.w) * (cd.x * v1 + cd.y * v2);
        pdfPos = 1 / (PI * worldCenterRadSqr.w);
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
    float3 lightDir;
    if(twoSided)
    {
        lightDir = sampleCosineWeightedSphere(randValuesDir);
    }
    else
    {
        lightDir = sampleCosineWeightedHemisphere(randValuesDir);
    }
    float3x3 tanToWS = constructBasisTransform(geometryNormalWS, tangent);

    output.radiance.setFromRGBUnbounded(surfaceDefRGB.emissive);
    output.pdfPos = 1.f / (primCount * area);
    output.pdfDir = twoSided ? pdfCosineWeightedSphere(lightDir.y) : pdfCosineWeightedHemisphere(lightDir.y);
    output.positionWS = pos;
    output.directionWS = mul(lightDir, tanToWS);
    output.normalWS = geometryNormalWS;
    output.instanceIndex = lightEntry.instanceIndex;
    output.primitiveIndex = primitiveIndex;
    output.baryCentrics = barycentrics.yz;
    output.sampledAsTwoSided = twoSided;

}

void sampleRandomLightPosition(float4 randValues, out LightSampleOutput output, out float pdfLightSelection)
{
    float selectLightRand = randValues.w;
    uint lightIndex = min((uint) floor(selectLightRand * g_lightCount), g_lightCount - 1);

    sampleLight(lightIndex, randValues.xyz, float2(0, 0), output);
    pdfLightSelection = 1.f / g_lightCount;
}


void pdfForSamplingLight(uint instanceIndex, uint primitiveIndex, float2 bary, float3 lightSurfaceNormal, float3 towardsDir, float envSampleRelativeProbability, out float lightPickPDF, out float posPDF, out float dirPDF)
{
    
    float lightProb = g_lightCount + envSampleRelativeProbability;
    lightPickPDF = 1.f / lightProb;
    
    uint2 matMeshIndices = getMaterialAndMeshIndicesForInstance(instanceIndex);
    RenderObjectTransformDataGPU transformData = getTransformDataForInstance(instanceIndex);
    MeshEntryGPU meshEntry = getMeshEntry(matMeshIndices.y);
    bool twoSided = (getMaterialEntry(matMeshIndices.x).materialMask & MaterialMask_TwoSided) != 0;
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
    
    if (twoSided)
    {
        dirPDF = pdfSphere();
    } 
    else
    {
        dirPDF = dot(lightSurfaceNormal, towardsDir) >= 0 ? pdfHemisphere() : 0;
    }

}



void pdfForSamplingEnv(float envSampleRelativeProbability, float3 towardsDir, out float lightPickPDF, out float posPDF, out float dirPDF)
{

    float lightProb = g_lightCount + envSampleRelativeProbability;
    lightPickPDF = envSampleRelativeProbability / lightProb;
    
    //float theta = acos(clamp(dir.y, -1, 1));
    //float sinTheta = sin(theta);
    float cosTheta = towardsDir.y;
    float sinTheta = sqrt(1.f - cosTheta * cosTheta);
    
    float4 worldCenterRadSqr = getWorldCenterAndRadiusSqr();
    dirPDF = safeDiv(1.f, (2.f * PI * PI * sinTheta));
    posPDF = 1 / (PI * worldCenterRadSqr.w);
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