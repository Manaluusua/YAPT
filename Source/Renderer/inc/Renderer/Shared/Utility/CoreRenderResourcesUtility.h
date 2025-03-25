#pragma once

#include <Gfx/GfxTypes.h>
#include <Renderer/Shared/Utility/RenderResourcesPool.h>
#include <Renderer/Shared/Utility/MultipleScatterLUT.h>
#include <Renderer/Shared/Utility/SpectralUtility.h>

namespace YAPT
{
    class CRenderer;
    enum class DefaultSamplerType
    {
        LINEAR_REPEAT,
        NEAREST_REPEAT,
        LINEAR_CLAMP,
        NEAREST_CLAMP
    };

    enum class DefaultBufferType
    {
        FULLSCREEN_PRIMITIVE_INDICES,
        FULLSCREEN_PRIMITIVE_POS,
        FULLSCREEN_PRIMITIVE_POS_UV
    };

    enum class DefaultTextureType
    {
        BLACK,
        WHITE,
        NOISE
    };

	class CoreRenderResourcesUtility
	{
    public:
        CoreRenderResourcesUtility(CRenderer* renderer);
        ~CoreRenderResourcesUtility();

        SamplerHandle getDefaultSampler(DefaultSamplerType type) const;
        BufferViewHandle getDefaultBufferView(DefaultBufferType type) const;
        TextureViewHandle getDefaultTextureView(DefaultTextureType type) const;

        const VertexBufferDefinition* getDefaultVertexBufferDefinition(DefaultBufferType type) const;

        MultiScatteringLUTs& getMultiScatteringLUTs() { return m_multiScatteringLUTs; }
        SpectralUtility& getSpectralUtility() { return m_specUtility; }
    private:

        struct BufferHandleAndView
        {
            BufferHandle buffer;
            BufferViewHandle bufferView;
        };

        struct TextureHandleAndView
        {
            TextureHandle texture;
            TextureViewHandle textureView;
        };


        void initialize();
        void deinitialize();

        void createBuffers();
        void createTextures();
        void createSamplers();

        void createBufferViews();
        void createTextureViews();

        void uploadBuffers();
        void uploadTextures();

        void unloadSamplers();
        MultiScatteringLUTs m_multiScatteringLUTs;
        SpectralUtility m_specUtility;

        CRenderer* m_renderer;
        GfxApiHandle m_gfx;
        

        BufferHandleAndView m_fullscreenPrimVertexBufferPos;
        BufferHandleAndView m_fullscreenPrimVertexBufferPosUv;
        BufferHandleAndView m_fullscreenPrimIndexBuffer;

        TextureHandleAndView m_blackTex;
        TextureHandleAndView m_whiteTex;
        TextureHandleAndView m_noiseTex;

        SamplerHandle m_linearSamplerRepeat;
        SamplerHandle m_nearestSamplerRepeat;
        SamplerHandle m_linearSamplerClamp;
        SamplerHandle m_nearestSamplerClamp;

        RenderResourcesPool m_postProcessPool;

        
       
	};
}
