#include <Renderer/Shared/Utility/SpectralUtility.h>
#include <Renderer/Shared/Utility/SpectralData.h>
#include <Renderer/Shared/CRenderer.h>
#include <Gfx/GfxBasicTypesUtility.h>

namespace YAPT
{
	constexpr ResourceFormat CIE_XYZ_FORMAT = ResourceFormat::RGB32_SFLOAT;
	constexpr int SPECTRAL_LAMBDA_MIN = c_cieLambda[0];
	constexpr int SPECTRAL_LAMBDA_MAX = c_cieLambda[SIE_SAMPLE_COUNT-1];
	constexpr int SPECTRAL_LAMBDA_STEP = (SPECTRAL_LAMBDA_MAX - SPECTRAL_LAMBDA_MIN) / SIE_SAMPLE_COUNT;

	SpectralUtility::SpectralUtility()
		:m_renderer(nullptr)
	{

	}
	SpectralUtility::~SpectralUtility()
	{

	}

	void SpectralUtility::initializeLUTStorage(CRenderer* renderer, RenderResourcesPool& pool)
	{
		m_renderer = renderer;
		{
			TextureDesc texDesc(ResourceDimension::TEXTURE_2D, CIE_XYZ_FORMAT, RESOURCE_USAGE_COPY_DESTINATION | RESOURCE_USAGE_SAMPLED_TEXTURE, SIE_SAMPLE_COUNT, 1, 1, 1);
			ResourceStateDescription resState{ RESOURCE_USAGE_UNKNOWN, ACCESS_FLAGS_READ, SHADERSTAGE_NONE };
			m_cieXYZColorMacthingLUT.texture = pool.requestTexture(texDesc, resState, "CIEXYZColorMatchingLUT");
		}
		
	}
	
	uint32_t SpectralUtility::getCIELUTMinLambda() const
	{
		return c_cieLambda[0];
	}
	uint32_t SpectralUtility::getCIELUTMaxLambda() const
	{
		return c_cieLambda[SIE_SAMPLE_COUNT - 1];
	}
	uint32_t SpectralUtility::getCIELUTLambdaStep() const
	{
		return SIE_SAMPLE_LAMBDA_STEP;
	}
	uint32_t SpectralUtility::getCIELUTSampleCount() const
	{
		return SIE_SAMPLE_COUNT;
	}

	float SpectralUtility::getIntegralCIEY() const
	{
		return SIE_Y_SUM;
	}

	void SpectralUtility::initializeLUTContents()
	{
		//CIE XYZ Deg2 color matching coeffs
		{

			std::vector<float> cieXYZCoeffs;
			cieXYZCoeffs.resize(SIE_SAMPLE_COUNT * 3);
			for (size_t i = 0; i < SIE_SAMPLE_COUNT; ++i)
			{
				cieXYZCoeffs[i * 3] = c_cieDeg2X[i];
				cieXYZCoeffs[i * 3 + 1] = c_cieDeg2Y[i];
				cieXYZCoeffs[i * 3 + 2] = c_cieDeg2Z[i];
			}

			TextureDataDefinition texData;
			texData.rowPitchInBytes = size_t(getFormatSizeInBytes(CIE_XYZ_FORMAT)) * SIE_SAMPLE_COUNT;
			texData.data = cieXYZCoeffs.data();
			Gfx::uploadTexture(m_renderer->getGfxHandle(), m_cieXYZColorMacthingLUT.texture, 0, 1, 0, 1, &texData, GpuUploadStage::BEFORE_RENDER);
			m_cieXYZColorMacthingLUT.textureView = Gfx::getTextureView(m_renderer->getGfxHandle(), m_cieXYZColorMacthingLUT.texture, { CIE_XYZ_FORMAT });
		}
		

	}


}