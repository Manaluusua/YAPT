#pragma once

#include <Math/MathUtility.h>

namespace YAPT
{

    //metallic fresnel using incident and grazing angle parametrization for complex IOR. From: "Artist Friendly Metallic Fresnel" by Ole Gulbrandsen
    float getConductorRefractiveIndex(float r, float g)
    {
        float sqrtR = sqrt(r);
        return  g * ((1 - r) / (1 + r)) + (1 - g) * ((1 + sqrtR) / (1 - sqrtR));
    }
    float getConductorExtinctionthroughputSquared(float r, float refractiveIndex) {
        float nr = (refractiveIndex + 1) * (refractiveIndex + 1) * r - (refractiveIndex - 1.0f) * (refractiveIndex - 1.0f);
        return nr / (1.0f - r);
    }


    glm::vec2 getConductorRefractiveIndexAndExtinctionthroughputSquared(float r, float g)
    {
        float rc = glm::clamp(r, 0.0f, 0.99f);
        float n = getConductorRefractiveIndex(rc, g);
        float k2 = getConductorExtinctionthroughputSquared(rc, n);

        return glm::vec2(n, k2);
    }

    //from: https://seblagarde.wordpress.com/2013/04/29/memo-on-fresnel-equations/
    glm::vec3 fresnelDielectricConductor(glm::vec3 etaReal, glm::vec3 etaImg, float cosTheta)
    {
        float cosTheta2 = cosTheta * cosTheta;
        float sinTheta2 = 1 - cosTheta2;
        glm::vec3 etaReal2 = etaReal * etaReal;
        glm::vec3 etaImg2 = etaImg * etaImg;

        glm::vec3 t0 = etaReal2 - etaImg2 - sinTheta2;
        glm::vec3 a2plusb2 = sqrt(t0 * t0 + 4.f * etaReal2 * etaImg2);
        glm::vec3 t1 = a2plusb2 + cosTheta2;
        glm::vec3 a = sqrt(0.5f * (a2plusb2 + t0));
        glm::vec3 t2 = 2.f * a * cosTheta2;
        glm::vec3 rs = (t1 - t2) / (t1 + t2);

        glm::vec3 t3 = cosTheta2 * a2plusb2 + sinTheta2 * sinTheta2;
        glm::vec3 t4 = t2 * sinTheta2;
        glm::vec3 rp = rs * (t3 - t4) / (t3 + t4);

        return 0.5f * (rp + rs);
    }


    float fresnelDielectricDielectric2(float eta, float cosTheta)
    {
        float c = MathUtils::saturate(cosTheta);
        float temp = eta * eta + c * c - 1.f;

        if (temp < 0.f)
        {
            return 1.f;
        }
            

        float g = sqrt(temp);
        return 0.5f * MathUtils::sqr((g - c) / max(g + c, 0.000001f)) *
            (1 + MathUtils::sqr(((g + c) * c - 1.f) / ((g - c) * c + 1.f)));
    }

    template<typename T>
    T fresnelSchlick(T reflectivity, float cosTheta)
    {
        return reflectivity + (1.0f - reflectivity) * pow(1.0f - cosTheta, 5);
    }

    template<typename T>
    T fresnelSchlick(T reflectivity, T edgeTint, float cosTheta)
    {
        return reflectivity + (edgeTint - reflectivity) * pow(1.0f - cosTheta, 5);
    }


}