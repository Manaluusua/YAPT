#pragma once
#include <Gfx/GfxTypes.h>
#include <Renderer/Shared/Utility/RenderResourcesPool.h>
#include <Math/Math.h>
#include <Renderer/Shared/Utility/SpectralData.h>
namespace YAPT
{
	class CRenderer;
	class SpectralUtility
	{
	public:
		enum class ColorSpace
		{
			SRGB,
			REC2020
		};

		SpectralUtility();
		~SpectralUtility();

		void initializeLUTStorage(CRenderer* r, RenderResourcesPool& pool);
		void initializeLUTContents();

		
		TextureViewHandle getXYZColorMatchingLUT() { return m_cieXYZColorMatchingLUT.textureView; }
		TextureViewHandle getSRGBToSPDLUT() { return m_srgbToSPDLUT.textureView; }
		TextureViewHandle getREC2020ToSPDLUT() { return m_rec2020ToSPDLUT.textureView; }

		static constexpr uint32_t getCIELUTMinLambda()
		{
			return CIE_LUT_LAMBDA_MIN;
		}

		static constexpr uint32_t getCIELUTMaxLambda()
		{
			return CIE_LUT_LAMBDA_MAX;
		}


		static constexpr uint32_t getCIELUTSampleCount()
		{
			return CIE_LUT_RESOLUTION;
		}

		
		static void generateSampleLambdas(float rand, size_t sampleCount, float* lambdaOut, float* pdfOut, float lambdaMin, float lambdaMax);

	private:

		bool loadRGBToSPDLUT(vec3p* lutData, ColorSpace s);
		void storeRGBToSPDLUT(vec3p* lutData, ColorSpace s);
		void calculateRGBToSPDLUT(vec3p* lutData, ColorSpace s, bool printError);

		uint32_t getResolution(ColorSpace s);

		bool tryToLoadFromfile(const char* filename, ResourceDimension dim, ResourceFormat f, const glm::uvec3& expectedDimensions, void* dataOut);

		const char* getFilename(ColorSpace s);

		struct TextureHandleAndView
		{
			TextureHandle texture;
			TextureViewHandle textureView;
		};
		TextureHandleAndView m_cieXYZColorMatchingLUT;
		TextureHandleAndView m_srgbToSPDLUT;
		TextureHandleAndView m_rec2020ToSPDLUT;


		CRenderer* m_renderer;
	};



}