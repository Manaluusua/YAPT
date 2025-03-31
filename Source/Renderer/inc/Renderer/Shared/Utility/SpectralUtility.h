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
			return c_cieLambda[0];
		}

		static constexpr uint32_t getCIELUTMaxLambda()
		{
			return c_cieLambda[CIE_SAMPLE_COUNT - 1];
		}

		static constexpr uint32_t getCIELUTLambdaStep()
		{
			return CIE_SAMPLE_LAMBDA_STEP;
		}

		static constexpr uint32_t getCIELUTSampleCount()
		{
			return CIE_SAMPLE_COUNT;
		}

		static constexpr float getIntegralCIEY()
		{
			return CIE_Y_SUM;
		}

		template<typename Vec2Type>
		static void generateSampleLambdas(float rand, size_t sampleCount, Vec2Type* lambdaPDF, float lambdaMin, float lambdaMax)
		{
			assert(lambdaMax > lambdaMin);
			float lambdaRange = lambdaMax - lambdaMin;
			float pdf = 1.f / lambdaRange;
			
			float lambdaStep = lambdaRange / sampleCount;
			float previousSample = lambdaMin + rand * lambdaRange;

			lambdaPDF[0] = Vec2Type(previousSample, pdf);

			for (size_t i = 1; i < sampleCount; ++i)
			{
				float lambda = previousSample + lambdaStep;
				if (lambda > lambdaMax)
				{
					lambda = lambdaMin + (lambda - lambdaMax);
				}
				lambdaPDF[i] = Vec2Type(lambda, pdf);
				previousSample = lambda;
			}
		}

	private:

		bool loadRGBToSPDLUT(vec3* lutData, ColorSpace s);
		void storeRGBToSPDLUT(vec3* lutData, ColorSpace s);
		void calculateRGBToSPDLUT(vec3* lutData, ColorSpace s, bool printError);

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