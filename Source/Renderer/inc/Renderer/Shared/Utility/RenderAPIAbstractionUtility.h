#pragma once

#include <Math/Math.h>

namespace YAPT
{
    struct FullScreenPrimitive
    {
        const vec3p* positions;
        const vec2p* uvCoordinates;
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
        static const vec3p p[] = { vec3p(-1.f, -1.f, 1.f), vec3p(3.f, -1.f, 1.f), vec3p(-1.f, 3.f, 1.f) };
        static const vec2p uv[] = { vec2p(0.f, 2.f), vec2p(2.f, 2.f), vec2p(0.f, 0.f) };
        static const uint32_t indices[] = { 0, 1, 2 };
#elif RENDERER_VK
        static const vec3p p[] = { vec3p(-1.f, -1.f, 1.f), vec3p(3.f, -1.f, 1.f), vec3p(-1.f, 3.f, 1.f) };
        static const vec2p uv[] = { vec2p(0.f, 0.f), vec2p(2.f, 0.f), vec2p(0.f, 2.f) };
        static const uint32_t indices[] = { 0, 2, 1 };
#else
#error "NOT IMPLEMENTED"
#endif
        static const FullScreenPrimitive fsp{
           p,
           uv,
           sizeof(p) / sizeof(vec3p),
           indices,
           sizeof(indices) / sizeof(uint32_t) };
        return fsp;
    }




}