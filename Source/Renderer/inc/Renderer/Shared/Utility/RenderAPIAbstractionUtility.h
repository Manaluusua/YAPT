#pragma once

#include <Math/Math.h>

namespace YAPT
{
    struct FullScreenPrimitive
    {
        const glm::vec3* positions;
        const glm::vec2* uvCoordinates;
        const size_t vertexCount;
        const uint32_t* indices;
        const size_t indexCount;

    };

    //static stuff
    inline const mat4& fromCommonNDCtoPlatformSpecificNDC()
    {
#ifdef RENDERER_DX12
        static const mat4 ndc = mat4(
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 0.5f, 0.0f,
            0.0f, 0.0f, 0.5f, 1.0f);
#elif RENDERER_VK
        static const mat4 ndc = mat4(
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, -1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 0.5f, 0.0f,
            0.0f, 0.0f, 0.5f, 1.0f);
#else
#error "NOT IMPLEMENTED"
#endif
        return ndc;
    }

    inline const mat4& fromPlatformNDCToTextureSpace()
    {
         
#ifdef RENDERER_DX12
        static const mat4 toUv = mat4(
            0.5f, 0.0f, 0.0f, 0.0f,
            0.0f, -0.5f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            0.5f, 0.5f, 0.0f, 1.0f);
#elif RENDERER_VK
        static const mat4 toUv = mat4(
            0.5f, 0.0f, 0.0f, 0.0f,
            0.0f, 0.5f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            0.5f, 0.5f, 0.0f, 1.0f);
#else
#error "NOT IMPLEMENTED"
#endif
        return toUv;
    }
     
    inline const FullScreenPrimitive& getFullscreenPrimitive()
    {
#ifdef RENDERER_DX12
        static const glm::vec3 p[] = { glm::vec3(-1.f, -1.f, 1.f), glm::vec3(3.f, -1.f, 1.f), glm::vec3(-1.f, 3.f, 1.f) };
        static const glm::vec2 uv[] = { glm::vec2(0.f, 2.f), glm::vec2(2.f, 2.f), glm::vec2(0.f, 0.f) };
        static const uint32_t indices[] = { 0, 1, 2 };
#elif RENDERER_VK
        static const glm::vec3 p[] = { glm::vec3(-1.f, -1.f, 1.f), glm::vec3(3.f, -1.f, 1.f), glm::vec3(-1.f, 3.f, 1.f) };
        static const glm::vec2 uv[] = { glm::vec2(0.f, 0.f), glm::vec2(2.f, 0.f), glm::vec2(0.f, 2.f) };
        static const uint32_t indices[] = { 0, 1, 2 };
#else
#error "NOT IMPLEMENTED"
#endif
        static const FullScreenPrimitive fsp{
           p,
           uv,
           sizeof(p) / sizeof(glm::vec3),
           indices,
           sizeof(indices) / sizeof(uint32_t) };
        return fsp;
    }




}