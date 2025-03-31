#include <Renderer/Shared/Utility/SpectralUtility.h>
#include <Common/Common.h>
#include <Renderer/Shared/CRenderer.h>
#include <Gfx/GfxBasicTypesUtility.h>
#include <glm/glm.hpp>
#include <array>
#include <limits>


using dvec3 = glm::dvec3;
using dmat3x3 = glm::dmat3x3;

namespace YAPT
{
	constexpr ResourceFormat CIE_XYZ_FORMAT = ResourceFormat::RGB32_SFLOAT;
	constexpr int SPECTRAL_LAMBDA_MIN = c_cieLambda[0];
	constexpr int SPECTRAL_LAMBDA_MAX = c_cieLambda[CIE_SAMPLE_COUNT-1];
	constexpr int SPECTRAL_LAMBDA_STEP = (SPECTRAL_LAMBDA_MAX - SPECTRAL_LAMBDA_MIN) / CIE_SAMPLE_COUNT;

	constexpr ResourceFormat RGB_TO_SPD_FORMAT = ResourceFormat::RGB32_SFLOAT;
	constexpr uint32_t RGB_TO_SPD_RESOLUTION = 32;
	constexpr uint32_t GAUSS_NEWTON_ITERATION_COUNT = 16;

	double gaussNewton(dvec3 expected, dvec3& coeffsInOut, size_t iterationCount, const dvec3* toRGBLUT);

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
			TextureDesc texDesc(ResourceDimension::TEXTURE_2D, CIE_XYZ_FORMAT, RESOURCE_USAGE_COPY_DESTINATION | RESOURCE_USAGE_SAMPLED_TEXTURE, CIE_SAMPLE_COUNT, 1, 1, 1);
			ResourceStateDescription resState{ RESOURCE_USAGE_UNKNOWN, ACCESS_FLAGS_READ, SHADERSTAGE_NONE };
			m_cieXYZColorMatchingLUT.texture = pool.requestTexture(texDesc, resState, "CIEXYZColorMatchingLUT");
		}

		{
			TextureDesc texDesc(ResourceDimension::TEXTURE_3D, RGB_TO_SPD_FORMAT, RESOURCE_USAGE_COPY_DESTINATION | RESOURCE_USAGE_SAMPLED_TEXTURE, RGB_TO_SPD_RESOLUTION, RGB_TO_SPD_RESOLUTION, 1, RGB_TO_SPD_RESOLUTION);
			ResourceStateDescription resState{ RESOURCE_USAGE_UNKNOWN, ACCESS_FLAGS_READ, SHADERSTAGE_NONE };
			m_srgbToSPDLUT.texture = pool.requestTexture(texDesc, resState, "SRGBToSPDLUT");
		}

		{
			TextureDesc texDesc(ResourceDimension::TEXTURE_3D, RGB_TO_SPD_FORMAT, RESOURCE_USAGE_COPY_DESTINATION | RESOURCE_USAGE_SAMPLED_TEXTURE, RGB_TO_SPD_RESOLUTION, RGB_TO_SPD_RESOLUTION, 1, RGB_TO_SPD_RESOLUTION);
			ResourceStateDescription resState{ RESOURCE_USAGE_UNKNOWN, ACCESS_FLAGS_READ, SHADERSTAGE_NONE };
			m_rec2020ToSPDLUT.texture = pool.requestTexture(texDesc, resState, "REC2020ToSPDLUT");
		}
		
	}

	void SpectralUtility::initializeLUTContents()
	{
		//CIE XYZ Deg2 color matching coeffs
		{

			std::vector<float> cieXYZCoeffs;
			cieXYZCoeffs.resize(CIE_SAMPLE_COUNT * 3);
			for (size_t i = 0; i < CIE_SAMPLE_COUNT; ++i)
			{
				cieXYZCoeffs[i * 3] = (float)c_cieDeg2X[i];
				cieXYZCoeffs[i * 3 + 1] = (float)c_cieDeg2Y[i];
				cieXYZCoeffs[i * 3 + 2] = (float)c_cieDeg2Z[i];
			}

			TextureDataDefinition texData;
			texData.rowPitchInBytes = size_t(getFormatSizeInBytes(CIE_XYZ_FORMAT)) * CIE_SAMPLE_COUNT;
			texData.data = cieXYZCoeffs.data();
			Gfx::uploadTexture(m_renderer->getGfxHandle(), m_cieXYZColorMatchingLUT.texture, 0, 1, 0, 1, &texData, GpuUploadStage::BEFORE_RENDER);
			m_cieXYZColorMatchingLUT.textureView = Gfx::getTextureView(m_renderer->getGfxHandle(), m_cieXYZColorMatchingLUT.texture, { CIE_XYZ_FORMAT });
		}

		//RGB to SPD (SRGB)
		{
			std::vector<vec3> rgbToSPD;
			rgbToSPD.resize(RGB_TO_SPD_RESOLUTION * RGB_TO_SPD_RESOLUTION * RGB_TO_SPD_RESOLUTION);
			if (!loadRGBToSPDLUT(rgbToSPD.data(), ColorSpace::SRGB))
			{
				calculateRGBToSPDLUT(rgbToSPD.data(), ColorSpace::SRGB, false);
				storeRGBToSPDLUT(rgbToSPD.data(), ColorSpace::SRGB);
			}

			//upload data
			TextureDataDefinition texData;
			texData.rowPitchInBytes = size_t(getFormatSizeInBytes(RGB_TO_SPD_FORMAT)) * RGB_TO_SPD_RESOLUTION;
			texData.data = rgbToSPD.data();
			Gfx::uploadTexture(m_renderer->getGfxHandle(), m_srgbToSPDLUT.texture, 0, 1, 0, 1, &texData, GpuUploadStage::BEFORE_RENDER);
			m_srgbToSPDLUT.textureView = Gfx::getTextureView(m_renderer->getGfxHandle(), m_srgbToSPDLUT.texture, { RGB_TO_SPD_FORMAT });

		}

		//RGB to SPD (REC2020)
		{
			std::vector<vec3> rgbToSPD;
			rgbToSPD.resize(RGB_TO_SPD_RESOLUTION * RGB_TO_SPD_RESOLUTION * RGB_TO_SPD_RESOLUTION);
			if (!loadRGBToSPDLUT(rgbToSPD.data(), ColorSpace::REC2020))
			{
				calculateRGBToSPDLUT(rgbToSPD.data(), ColorSpace::REC2020, false);
				storeRGBToSPDLUT(rgbToSPD.data(), ColorSpace::REC2020);
			}

			//upload data
			TextureDataDefinition texData;
			texData.rowPitchInBytes = size_t(getFormatSizeInBytes(RGB_TO_SPD_FORMAT)) * RGB_TO_SPD_RESOLUTION;
			texData.data = rgbToSPD.data();
			Gfx::uploadTexture(m_renderer->getGfxHandle(), m_rec2020ToSPDLUT.texture, 0, 1, 0, 1, &texData, GpuUploadStage::BEFORE_RENDER);
			m_rec2020ToSPDLUT.textureView = Gfx::getTextureView(m_renderer->getGfxHandle(), m_rec2020ToSPDLUT.texture, { RGB_TO_SPD_FORMAT });

		}
		
	}

	const char* SpectralUtility::getFilename(ColorSpace s)
	{
		switch (s)
		{
		case YAPT::SpectralUtility::ColorSpace::SRGB:
			return "sRGBToSPD";
		case YAPT::SpectralUtility::ColorSpace::REC2020:
			return "rec2020ToSPD";
		default:
			return "unknown_colorspace";
		}
	}

	bool SpectralUtility::loadRGBToSPDLUT(vec3* lutData, ColorSpace s)
	{

		bool success = tryToLoadFromfile(getFilename(s), ResourceDimension::TEXTURE_3D, RGB_TO_SPD_FORMAT, glm::uvec3(RGB_TO_SPD_RESOLUTION, RGB_TO_SPD_RESOLUTION, RGB_TO_SPD_RESOLUTION), lutData);
		return success;
	}
	void SpectralUtility::storeRGBToSPDLUT(vec3* lutData, ColorSpace s)
	{
		RendererCacheProvider* cache = m_renderer->getCacheProvider();
		if (!cache)
		{
			return;
		}
		RendererCacheProvider::ImageDefinition def;
		def.width = RGB_TO_SPD_RESOLUTION;
		def.height = RGB_TO_SPD_RESOLUTION;
		def.depthOrSlices = RGB_TO_SPD_RESOLUTION;
		def.format = RGB_TO_SPD_FORMAT;
		def.dimension = ResourceDimension::TEXTURE_3D;
		def.mips = 1;

		cache->storeImage(getFilename(s), def, lutData);
	}

	void SpectralUtility::calculateRGBToSPDLUT(vec3* lutDataOut, ColorSpace s, bool printError)
	{
		//generate conversion LUT
		std::vector<dvec3> conversionLut;
		conversionLut.resize(CIE_SAMPLE_COUNT);

		{
			struct Batch
			{
				size_t batchSize;
				size_t batchOffsets;
				size_t resolution;
				const double* rgbSpace;
				dvec3* output;
			};

			constexpr size_t BATCHES_COUNT = 8;
			std::array<Batch, BATCHES_COUNT> batches;

			uint32_t chunkSize = (CIE_SAMPLE_COUNT + BATCHES_COUNT - 1) / BATCHES_COUNT;
			const double* rgbSpace = s == ColorSpace::REC2020 ? c_rec2020XYZToRGB : c_srgbXYZToRGB;

			for (uint32_t i = 0; i < BATCHES_COUNT; ++i)
			{
				Batch& b = batches[i];
				b.batchSize = chunkSize;
				b.resolution = CIE_SAMPLE_COUNT;
				b.batchOffsets = b.batchSize * i;
				b.rgbSpace = rgbSpace;
				b.output = conversionLut.data();
			}
			

			auto calculateToRGBMapping = [](void* usrData)
			{
				Batch* item = static_cast<Batch*>(usrData);
				size_t resolution = item->resolution;
				size_t offset = item->batchOffsets;
				size_t batchSize = item->batchSize;

				dmat3x3 xyzToRGB;
				{
					const double* conv = item->rgbSpace;
					double* dst = glm::value_ptr(xyzToRGB);
					for (uint32_t i = 0; i < 9; ++i)
					{
						dst[i] = conv[i];
					}
					xyzToRGB = glm::transpose(xyzToRGB);
					
				}

				for (size_t i = offset; i < min(offset + batchSize, resolution); ++i)
				{
					dvec3 xyz = dvec3(c_cieDeg2X[i], c_cieDeg2Y[i], c_cieDeg2Z[i]) * c_cieD65StandardIllum[i] / CIE_D65_SUM;
					dvec3 rgb = xyzToRGB * xyz;

					item->output[i] = rgb;
				}

			};

			for (size_t i = 0; i < BATCHES_COUNT; ++i)
			{
				m_renderer->getThreadPool().addTask(calculateToRGBMapping, &batches[i]);
			}
			m_renderer->getThreadPool().waitForAllTasksCompleted();
		}

		//Generate LUT Data
		std::vector<double> lutErrors;
		lutErrors.resize(RGB_TO_SPD_RESOLUTION * RGB_TO_SPD_RESOLUTION * RGB_TO_SPD_RESOLUTION);

		{
			struct Batch
			{
				uvec3 batchSize;
				uvec3 batchOffsets;
				uvec3 resolution;
				vec3* output;
				double* errors;
				dvec3* toRGBLUT;
			};

			constexpr size_t BATCHES_COUNT = 8;
			std::array<Batch, BATCHES_COUNT> batches;

			uint32_t chunkSize = (RGB_TO_SPD_RESOLUTION + BATCHES_COUNT - 1) / BATCHES_COUNT;

			for (uint32_t i = 0; i < BATCHES_COUNT; ++i)
			{
				Batch& b = batches[i];
				b.batchSize = uvec3(RGB_TO_SPD_RESOLUTION, RGB_TO_SPD_RESOLUTION, chunkSize);
				b.resolution = uvec3(RGB_TO_SPD_RESOLUTION);
				b.batchOffsets = uvec3(0, 0, b.batchSize.z * i);
				b.output = lutDataOut;
				b.errors = lutErrors.data();
				b.toRGBLUT = conversionLut.data();
			}


			auto calculateRGBToSPDLUT = [](void* usrData)
			{
				Batch* item = static_cast<Batch*>(usrData);
				uvec3 resolution = item->resolution;
				uvec3 offset = item->batchOffsets;
				uvec3 batchSize = item->batchSize;
				dvec3 dresMinusOne = resolution - uvec3(1);

				for (uint32_t z = offset.z; z < min(offset.z + batchSize.z, resolution.z); ++z)
				{
					for (uint32_t y = offset.y; y < min(offset.y + batchSize.y, resolution.y); ++y)
					{
						for (uint32_t x = offset.x; x < min(offset.x + batchSize.x, resolution.x); ++x)
						{
							dvec3 rgb = dvec3(x, y, z) / dresMinusOne;
							dvec3 res = dvec3(0.0);
							double error = gaussNewton(rgb, res, GAUSS_NEWTON_ITERATION_COUNT, item->toRGBLUT);
							size_t outputIndex = z * (resolution.x * resolution.y) + y * resolution.x + x;

							item->output[outputIndex] = res;
							item->errors[outputIndex] = error;
						}
					}
				}
			};

			for (size_t i = 0; i < BATCHES_COUNT; ++i)
			{
				m_renderer->getThreadPool().addTask(calculateRGBToSPDLUT, &batches[i]);
			}
			m_renderer->getThreadPool().waitForAllTasksCompleted();

			if (printError)
			{
				double smallest = std::numeric_limits<double>::max();
				double biggest = 0;

				for (size_t i = 0; i < lutErrors.size(); ++i)
				{
					double r = lutErrors[i];
					if (r < smallest)
					{
						smallest = r;
					}
					if (r > biggest)
					{
						biggest = r;
					}
				}

				printf("RGB to SPD errors min: %f, max %f", smallest, biggest);
			}

			
		}
		
	}

	
	bool SpectralUtility::tryToLoadFromfile(const char* filename, ResourceDimension dim, ResourceFormat f, const glm::uvec3& expectedDimensions, void* dataOut)
	{
		RendererCacheProvider* cache = m_renderer->getCacheProvider();
		if (!cache)
		{
			return false;
		}

		RendererCacheProvider::ImageDefinition def;
		def.width = expectedDimensions.x;
		def.height = expectedDimensions.y;
		def.depthOrSlices = expectedDimensions.z;
		def.format = f;
		def.dimension = dim;
		def.mips = 1;

		bool succesful = cache->loadImage(filename, def, dataOut);
		return succesful;
	}

	


	//RGB to SPD LUT generation
	double sigmoid(double x) 
	{
		return 0.5 * x / std::sqrt(1.0 + x * x) + 0.5;
	}

	dvec3 calculateResiduals(dvec3 expected, dvec3 coeffs, const dvec3* toRGBLUT)
	{
		/*dvec3 samplePoints = {1.0f, 5.0f, 10.f};
		dvec3 m = expected * dvec3{2, 3, -2};
		dvec3 expectedCalculated = { 
			m[0] * pow(samplePoints[0], 2) + m[1] * samplePoints[0] + m[2],
			m[0] * pow(samplePoints[1], 2) + m[1] * samplePoints[1] + m[2],
			m[0] * pow(samplePoints[2], 2) + m[1] * samplePoints[2] + m[2] };
		dvec3 estimated = { 0.0, 0.0, 0.0 };
		for (uint32_t i = 0; i < 3; ++i)
		{
			for (uint32_t k = 0; k < 3; ++k)
			{
				estimated[i] = estimated[i] * samplePoints[i] + coeffs[k];
			}
		}
		return expectedCalculated - estimated;
		*/
		
		dvec3 estimated = { 0.0, 0.0, 0.0 };
		for (size_t i = 0; i < CIE_SAMPLE_COUNT; ++i)
		{
			double lambda = double(i) / (CIE_SAMPLE_COUNT - 1);
			double polynomial = 0;
			for (uint32_t k = 0; k < 3; ++k)
			{
				polynomial = polynomial * lambda + coeffs[k];
			}

			double s = sigmoid(polynomial);

			estimated += s * toRGBLUT[i];
		}
		 //TODO: pbrt recommends transforming to cie lab colorspace before calculating residue 
		return expected - estimated;
		
	}

	dmat3x3 calculateJacobian(dvec3 expected, dvec3 coeffs, const dvec3* toRGBLUT)
	{
		const double FINITE_DIFF_EPSILON = 1e-4;
		dvec3 residuals0;
		dvec3 residuals1;

		double diffDenom = 1.0 / (2.0 * FINITE_DIFF_EPSILON);

		dmat3x3 jacobianOut;

		for (uint32_t i = 0; i < 3; ++i)
		{
			dvec3 delta = {0.0f, 0.0f, 0.0f};
			delta[i] = FINITE_DIFF_EPSILON;
			residuals0 = calculateResiduals(expected, coeffs - delta, toRGBLUT);
			residuals1 = calculateResiduals(expected, coeffs + delta, toRGBLUT);

			jacobianOut[i] = (residuals1 - residuals0) * diffDenom;
		}
		return jacobianOut;
	}

	double gaussNewton(dvec3 expected, dvec3& coeffsInOut, size_t iterationCount, const dvec3* toRGBLUT)
	{
		dvec3 coeffs = coeffsInOut;
		double sqrError = 0;
		const double ERROR_THRESHOLD = 1e-6;
		for (size_t i = 0; i < iterationCount; ++i)
		{
			dvec3 residuals = calculateResiduals(expected, coeffs, toRGBLUT);
			dmat3x3 jacobian = calculateJacobian(expected, coeffs, toRGBLUT);

			//dmat3x3 jacobianTranspose = glm::transpose(jacobian);
			//dmat3x3 inverse = glm::inverse(jacobianTranspose * jacobian);
			//dvec3 iterationDelta = inverse * jacobianTranspose * residuals;

			dvec3 iterationDelta = glm::inverse(jacobian) * residuals;

			coeffs -= iterationDelta;

			sqrError = glm::dot(residuals, residuals);

			if (sqrError < ERROR_THRESHOLD)
			{
				break;
			}

			double maxCoeff = max(max(coeffs[0], coeffs[1]), coeffs[2]);

			if (maxCoeff > 200)
			{
				for (uint32_t k = 0; k < 3; ++k)
				{
					coeffs[k] *= 200 / maxCoeff;
				}
			}
		}


		coeffsInOut = coeffs;
		return sqrError;
	}

}