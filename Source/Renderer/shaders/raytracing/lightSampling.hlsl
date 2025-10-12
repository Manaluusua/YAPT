#ifndef RAYTRACE_LIGHT_SAMPLING_INCL
#define RAYTRACE_LIGHT_SAMPLING_INCL

#include "../common/miscBrdf.hlsl"


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

    radianceOut.setFromRGBUnbounded(getSkyBoxColor(-lightDir, g_envType, g_envTexIndex).xyz);
    posOut = lightPos;
    dirOut = lightDir;
    pdfPosOut = pdfPos;
    pdfDirOut = pdfDir;

}

void sampleLight(uint lightIndex, float4 randValues0, float2 randValues1, out SpectralSamples radianceOut, out float3 posOut, out float3 dirOut, out float3 normalOut, out float pdfPosOut, out float pdfDirOut)
{
    LightEntryGPU lightEntry = g_lights[lightIndex];
    MeshEntryGPU meshEntry = getMeshEntry(lightEntry.meshIndex);

    uint primCount = (meshEntry.indexCount / 3);
    uint primitiveIndex = min(primCount * randValues0.z, primCount - 1);

    float3 barycentrics = float3(1 - randValues0.x - randValues0.y, randValues0.x, randValues0.y);
    uint3 indices = fetchIndices(meshEntry.indexBuffer, primitiveIndex);

    float3 p1, p2, p3;
    fetchMeshPositions(meshEntry.positionBuffer, indices, p1, p2, p3);
    p1 = mul(lightEntry.transform, float4(p1, 1)).xyz;
    p2 = mul(lightEntry.transform, float4(p2, 1)).xyz;
    p3 = mul(lightEntry.transform, float4(p3, 1)).xyz;
    float3 geometryNormal = cross(p2 - p1, p3 - p1);
    float geomNormalLength = length(geometryNormal);
    geometryNormal /= geomNormalLength;
    
    float area = geomNormalLength * 0.5f;
    float3 pos = barycentrics.x * p1 + barycentrics.y * p2 + barycentrics.z * p3;
    
    float3 tangent = meshHasValidTangents(meshEntry.tangentBuffer) ? fetchMeshTangent(meshEntry.tangentBuffer, indices, barycentrics) : float3(1.f, 0.f, 0.f);
    float2 uv = meshHasValidUVs(meshEntry.uvBuffer) ? fetchMeshUV(meshEntry.uvBuffer, indices, barycentrics) : float2(0.5f, 0.5f);

    MaterialEntryGPU matEntry = getMaterialEntry(lightEntry.matIndex);
    SurfaceDefinitionRGB surfaceDefRGB;
    fetchSurfaceMaterialParameters(matEntry, surfaceDefRGB);
    modifySurfaceEmissionWithTexture(matEntry, uv, surfaceDefRGB);

    geometryNormal = mul(lightEntry.transformInvTransp, float4(geometryNormal, 0.f)).xyz;
    tangent = mul(lightEntry.transformInvTransp, float4(tangent, 0.f)).xyz;
    
    float3x3 tanToWS = constructBasisTransform(geometryNormal, tangent);
    float3 lightDir = sampleHemisphere(randValues1);
    lightDir = mul(tanToWS, lightDir);

	

    radianceOut.setFromRGBUnbounded(surfaceDefRGB.emissive);
    
    pdfPosOut = 1.f / (primCount * area);
    pdfDirOut = pdfHemisphere();
    
    posOut = pos;
    dirOut = lightDir;
    normalOut = geometryNormal;

}

float areaDensityMultiplier(float3 fromToUnnormalized, float3 toNormal)
{
    float invDistSqr = 1.f / dot(fromToUnnormalized, fromToUnnormalized);
    float absDot = abs(dot(toNormal, fromToUnnormalized * sqrt(invDistSqr)));
    return absDot * invDistSqr;
}


void sampleLightOrEnv(float lightPickRand, float4 lightSampleRand0, float2 lightSampleRand1, float envSampleRelativeProbability, out SpectralSamples radianceOut, out float3 posOut, out float3 dirOut, out float3 normalOut, out float pdfPosOut, out float pdfDirOut, out float pdflightSelection, out bool sampledEnvironment)
{
    if (g_lightCount == 0)
    {
        sampleEnvironmentLighting(lightSampleRand0, radianceOut, posOut, dirOut, pdfPosOut, pdfDirOut);
        pdflightSelection = 1.f;
        sampledEnvironment = true;
        normalOut = 0;
        return;
    }

    float lightProb = g_lightCount + envSampleRelativeProbability;
    float s = lightPickRand * lightProb;
    if (s > g_lightCount)
    {
        sampleEnvironmentLighting(lightSampleRand0, radianceOut, posOut, dirOut, pdfPosOut, pdfDirOut);
        pdflightSelection = envSampleRelativeProbability / lightProb;
        sampledEnvironment = true;
        normalOut = 0;

    }
    else
    {
        uint lightIndex = min((uint) floor(lightPickRand * g_lightCount), g_lightCount - 1);
        sampleLight(lightIndex, lightSampleRand0, lightSampleRand1, radianceOut, posOut, dirOut, normalOut, pdfPosOut, pdfDirOut);
        pdflightSelection = 1 / lightProb;
        sampledEnvironment = false;

    }
}

void pdfForSamplingLight(uint instanceIndex, uint primitiveIndex, float2 bary, float3 lightSurfaceNormal, float3 towardsDir, float envSampleRelativeProbability, out float lightPickPDF, out float posPDF, out float dirPDF)
{
    
    float lightProb = g_lightCount + envSampleRelativeProbability;
    lightPickPDF = 1.f / lightProb;
    
    uint2 matMeshIndices = getMaterialAndMeshIndicesForInstance(instanceIndex);
    RenderObjectTransformDataGPU transformData = getTransformDataForInstance(instanceIndex);
    MeshEntryGPU meshEntry = getMeshEntry(matMeshIndices.y);
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
    dirPDF = dot(lightSurfaceNormal, towardsDir) >= 0 ? pdfHemisphere() : 0;

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




#endif