#pragma once
#include <Gfx/GfxTypes.h>
#include <Renderer/Shared/Utility/RenderResourcesPool.h>
#include <Math/Math.h>

namespace YAPT
{
	class CRenderer;
	class SpectralUtility
	{
	public:
		SpectralUtility();
		~SpectralUtility();

		void initializeLUTStorage(CRenderer* r, RenderResourcesPool& pool);
		void initializeLUTContents();

		uint32_t getCIELUTMinLambda() const;
		uint32_t getCIELUTMaxLambda() const;
		uint32_t getCIELUTLambdaStep() const;
		uint32_t getCIELUTSampleCount() const;
		float getIntegralCIEY() const;
		TextureViewHandle getXYZColorMatchingLUT() { return m_cieXYZColorMacthingLUT.textureView; }
	private:
		struct TextureHandleAndView
		{
			TextureHandle texture;
			TextureViewHandle textureView;
		};



		TextureHandleAndView m_cieXYZColorMacthingLUT;


		CRenderer* m_renderer;
	};



}