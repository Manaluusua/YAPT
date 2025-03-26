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
		SpectralUtility();
		~SpectralUtility();

		void initializeLUTStorage(CRenderer* r, RenderResourcesPool& pool);
		void initializeLUTContents();

		
		TextureViewHandle getXYZColorMatchingLUT() { return m_cieXYZColorMacthingLUT.textureView; }

		static constexpr uint32_t getCIELUTMinLambda()
		{
			return c_cieLambda[0];
		}

		static constexpr uint32_t getCIELUTMaxLambda()
		{
			return c_cieLambda[SIE_SAMPLE_COUNT - 1];
		}

		static constexpr uint32_t getCIELUTLambdaStep()
		{
			return SIE_SAMPLE_LAMBDA_STEP;
		}

		static constexpr uint32_t getCIELUTSampleCount()
		{
			return SIE_SAMPLE_COUNT;
		}

		static constexpr float getIntegralCIEY()
		{
			return SIE_Y_SUM;
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
		struct TextureHandleAndView
		{
			TextureHandle texture;
			TextureViewHandle textureView;
		};



		TextureHandleAndView m_cieXYZColorMacthingLUT;


		CRenderer* m_renderer;
	};



}