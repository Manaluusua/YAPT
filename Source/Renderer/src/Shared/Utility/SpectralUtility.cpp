#include <Renderer/Shared/Utility/SpectralUtility.h>
#include <Common/Common.h>
#include <Renderer/Shared/CRenderer.h>
#include <Gfx/GfxBasicTypesUtility.h>
#include <glm/glm.hpp>
#include <array>
#include <limits>
#include <spectralConstants.h>

using dvec3 = glm::dvec3;
using dmat3x3 = glm::dmat3x3;

namespace YAPT
{
	constexpr ResourceFormat CIE_XYZ_FORMAT = ResourceFormat::RGB32_SFLOAT;
	constexpr int SPECTRAL_LAMBDA_MIN = CIE_LUT_LAMBDA_MIN;
	constexpr int SPECTRAL_LAMBDA_MAX = CIE_LUT_LAMBDA_MAX;
	constexpr int SPECTRAL_LAMBDA_STEP = (SPECTRAL_LAMBDA_MAX - SPECTRAL_LAMBDA_MIN) / CIE_LUT_RESOLUTION;

	constexpr ResourceFormat RGB_TO_SPD_FORMAT = ResourceFormat::RGB32_SFLOAT;

	constexpr uint32_t GAUSS_NEWTON_ITERATION_COUNT = 16;

	double gaussNewton(dvec3 expected, dvec3& coeffsInOut, size_t iterationCount, const dmat3x3& toXyz, const dvec3* toRGBLUT);
	dmat3x3 toMatrix(const double* valArray);

	SpectralUtility::SpectralUtility()
		:m_renderer(nullptr)
	{

	}
	SpectralUtility::~SpectralUtility()
	{

	}


	void SpectralUtility::generateSampleLambdas(float rand, size_t sampleCount, float* lambdaOut, float* pdfOut, float lambdaMin, float lambdaMax)
	{
		assert(lambdaMax > lambdaMin);
		float lambdaRange = lambdaMax - lambdaMin;
		float pdf = 1.f / lambdaRange;

		float lambdaStep = lambdaRange / sampleCount;
		float previousSample = lambdaMin + rand * lambdaRange;

		lambdaOut[0] = previousSample;
		pdfOut[0] = pdf;

		for (size_t i = 1; i < sampleCount; ++i)
		{
			float lambda = previousSample + lambdaStep;
			if (lambda > lambdaMax)
			{
				lambda = lambdaMin + (lambda - lambdaMax);
			}
			pdfOut[i] = pdf;
			lambdaOut[i] = lambda;
			previousSample = lambda;
		}
	}

	void SpectralUtility::initializeLUTStorage(CRenderer* renderer, RenderResourcesPool& pool)
	{
		m_renderer = renderer;
		{
			TextureDesc texDesc(ResourceDimension::TEXTURE_1D, CIE_XYZ_FORMAT, RESOURCE_USAGE_COPY_DESTINATION | RESOURCE_USAGE_SAMPLED_TEXTURE, CIE_LUT_RESOLUTION, 1, 1, 1);
			ResourceStateDescription resState{ RESOURCE_USAGE_UNKNOWN, ACCESS_FLAGS_READ, SHADERSTAGE_NONE };
			m_cieXYZColorMatchingLUT.texture = pool.requestTexture(texDesc, resState, "CIEXYZColorMatchingLUT");
		}

		{
			TextureDesc texDesc(ResourceDimension::TEXTURE_3D, RGB_TO_SPD_FORMAT, RESOURCE_USAGE_COPY_DESTINATION | RESOURCE_USAGE_SAMPLED_TEXTURE, SRGB_TO_SPD_RES, SRGB_TO_SPD_RES, 1, SRGB_TO_SPD_RES);
			ResourceStateDescription resState{ RESOURCE_USAGE_UNKNOWN, ACCESS_FLAGS_READ, SHADERSTAGE_NONE };
			m_srgbToSPDLUT.texture = pool.requestTexture(texDesc, resState, "SRGBToSPDLUT");
		}

		{
			TextureDesc texDesc(ResourceDimension::TEXTURE_3D, RGB_TO_SPD_FORMAT, RESOURCE_USAGE_COPY_DESTINATION | RESOURCE_USAGE_SAMPLED_TEXTURE, REC2020_TO_SPD_RES, REC2020_TO_SPD_RES, 1, REC2020_TO_SPD_RES);
			ResourceStateDescription resState{ RESOURCE_USAGE_UNKNOWN, ACCESS_FLAGS_READ, SHADERSTAGE_NONE };
			m_rec2020ToSPDLUT.texture = pool.requestTexture(texDesc, resState, "REC2020ToSPDLUT");
		}
		
	}

	void SpectralUtility::initializeLUTContents()
	{
		//CIE XYZ Deg2 color matching coeffs
		{

			std::vector<float> cieXYZCoeffs;
			cieXYZCoeffs.resize(CIE_LUT_RESOLUTION * 3);
			for (size_t i = 0; i < CIE_LUT_RESOLUTION; ++i)
			{
				size_t index = CIE_LUT_ARRAY_OFFSET + i;
				cieXYZCoeffs[i * 3] = (float)c_cieDeg2X[index];
				cieXYZCoeffs[i * 3 + 1] = (float)c_cieDeg2Y[index];
				cieXYZCoeffs[i * 3 + 2] = (float)c_cieDeg2Z[index];
			}

			TextureDataDefinition texData;
			texData.rowPitchInBytes = size_t(getFormatSizeInBytes(CIE_XYZ_FORMAT)) * CIE_LUT_RESOLUTION;
			texData.data = cieXYZCoeffs.data();
			Gfx::uploadTexture(m_renderer->getGfxHandle(), m_cieXYZColorMatchingLUT.texture, 0, 1, 0, 1, &texData, GpuUploadStage::BEFORE_RENDER);
			m_cieXYZColorMatchingLUT.textureView = Gfx::getTextureView(m_renderer->getGfxHandle(), m_cieXYZColorMatchingLUT.texture, { CIE_XYZ_FORMAT });
		}

		//RGB to SPD (SRGB)
		{
			std::vector<vec3p> rgbToSPD;
			rgbToSPD.resize(SRGB_TO_SPD_RES * SRGB_TO_SPD_RES * SRGB_TO_SPD_RES);
			if (!loadRGBToSPDLUT(rgbToSPD.data(), ColorSpace::SRGB))
			{
				YAPT_LOG_DEBUG("Generating SRGB2XYZ LUT");
				calculateRGBToSPDLUT(rgbToSPD.data(), ColorSpace::SRGB, false);
				storeRGBToSPDLUT(rgbToSPD.data(), ColorSpace::SRGB);
			}

			//upload data
			TextureDataDefinition texData;
			texData.rowPitchInBytes = size_t(getFormatSizeInBytes(RGB_TO_SPD_FORMAT)) * SRGB_TO_SPD_RES;
			texData.data = rgbToSPD.data();
			Gfx::uploadTexture(m_renderer->getGfxHandle(), m_srgbToSPDLUT.texture, 0, 1, 0, 1, &texData, GpuUploadStage::BEFORE_RENDER);
			m_srgbToSPDLUT.textureView = Gfx::getTextureView(m_renderer->getGfxHandle(), m_srgbToSPDLUT.texture, { RGB_TO_SPD_FORMAT });

		}

		//RGB to SPD (REC2020)
		{
			std::vector<vec3p> rgbToSPD;
			rgbToSPD.resize(REC2020_TO_SPD_RES * REC2020_TO_SPD_RES * REC2020_TO_SPD_RES);
			if (!loadRGBToSPDLUT(rgbToSPD.data(), ColorSpace::REC2020))
			{
				YAPT_LOG_DEBUG("Generating REC20202XYZ LUT");
				calculateRGBToSPDLUT(rgbToSPD.data(), ColorSpace::REC2020, false);
				storeRGBToSPDLUT(rgbToSPD.data(), ColorSpace::REC2020);
			}

			//upload data
			TextureDataDefinition texData;
			texData.rowPitchInBytes = size_t(getFormatSizeInBytes(RGB_TO_SPD_FORMAT)) * REC2020_TO_SPD_RES;
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

	uint32_t SpectralUtility::getResolution(ColorSpace s)
	{
		uint32_t res = 0;
		switch (s)
		{
		case YAPT::SpectralUtility::ColorSpace::SRGB:
			res = SRGB_TO_SPD_RES;
			break;
		case YAPT::SpectralUtility::ColorSpace::REC2020:
			res = REC2020_TO_SPD_RES;
			break;
		default:
			assert(!"unknown color space");
			break;
		}
		return res;
	}

	bool SpectralUtility::loadRGBToSPDLUT(vec3p* lutData, ColorSpace s)
	{

		uint32_t res = getResolution(s);

		bool success = tryToLoadFromfile(getFilename(s), ResourceDimension::TEXTURE_3D, RGB_TO_SPD_FORMAT, glm::uvec3(res, res, res), lutData);
		return success;
	}
	void SpectralUtility::storeRGBToSPDLUT(vec3p* lutData, ColorSpace s)
	{
		RendererCacheProvider* cache = m_renderer->getCacheProvider();
		if (!cache)
		{
			return;
		}

		uint32_t res = getResolution(s);


		RendererCacheProvider::ImageDefinition def;
		def.width = res;
		def.height = res;
		def.depthOrSlices = res;
		def.format = RGB_TO_SPD_FORMAT;
		def.dimension = ResourceDimension::TEXTURE_3D;
		def.mips = 1;

		cache->storeImage(getFilename(s), def, lutData);
	}

	void SpectralUtility::calculateRGBToSPDLUT(vec3p* lutDataOut, ColorSpace s, bool printError)
	{
		uint32_t rgbTableRes = getResolution(s);;

		//generate conversion LUT
		std::vector<dvec3> conversionLut;
		conversionLut.resize(CIE_LUT_RESOLUTION);

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

			uint32_t chunkSize = (CIE_LUT_RESOLUTION + BATCHES_COUNT - 1) / BATCHES_COUNT;
			const double* rgbSpace = s == ColorSpace::REC2020 ? c_rec2020XYZToRGB : c_srgbXYZToRGB;

			for (uint32_t i = 0; i < BATCHES_COUNT; ++i)
			{
				Batch& b = batches[i];
				b.batchSize = chunkSize;
				b.resolution = CIE_LUT_RESOLUTION;
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

				dmat3x3 xyzToRGB = toMatrix(item->rgbSpace);
				

				for (size_t i = offset; i < min(offset + batchSize, resolution); ++i)
				{
					size_t index = CIE_LUT_ARRAY_OFFSET + i;
					dvec3 xyz = dvec3(c_cieDeg2X[index], c_cieDeg2Y[index], c_cieDeg2Z[index]) * c_cieD65StandardIllum[index] / CIE_D65_SUM;
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
		lutErrors.resize(rgbTableRes * rgbTableRes * rgbTableRes);

		{
			struct Batch
			{
				uvec3 batchSize;
				uvec3 batchOffsets;
				uvec3 resolution;
				vec3p* output;
				double* errors;
				dvec3* toRGBLUT;
				ColorSpace colorSpace;
			};

			constexpr size_t BATCHES_COUNT = 8;
			std::array<Batch, BATCHES_COUNT> batches;

			uint32_t chunkSize = (rgbTableRes + BATCHES_COUNT - 1) / BATCHES_COUNT;

			for (uint32_t i = 0; i < BATCHES_COUNT; ++i)
			{
				Batch& b = batches[i];
				b.batchSize = uvec3(rgbTableRes, rgbTableRes, chunkSize);
				b.resolution = uvec3(rgbTableRes);
				b.batchOffsets = uvec3(0, 0, b.batchSize.z * i);
				b.output = lutDataOut;
				b.errors = lutErrors.data();
				b.toRGBLUT = conversionLut.data();
				b.colorSpace = s;
			}


			auto calculateRGBToSPDLUT = [](void* usrData)
			{
				Batch* item = static_cast<Batch*>(usrData);
				uvec3 resolution = item->resolution;
				uvec3 offset = item->batchOffsets;
				uvec3 batchSize = item->batchSize;
				dvec3 dresMinusOne = resolution - uvec3(1);

				dmat3x3 toXyz;
				switch (item->colorSpace)
				{
				case YAPT::SpectralUtility::ColorSpace::SRGB:
					toXyz = toMatrix(c_srgbRGBToXYZ);
					break;
				case YAPT::SpectralUtility::ColorSpace::REC2020:
					toXyz = toMatrix(c_rec2020RGBToXYZ);
					break;
				default:
					assert(!"unknown colorspace");
					break;
				}

				for (uint32_t z = offset.z; z < min(offset.z + batchSize.z, resolution.z); ++z)
				{
					for (uint32_t y = offset.y; y < min(offset.y + batchSize.y, resolution.y); ++y)
					{
						for (uint32_t x = offset.x; x < min(offset.x + batchSize.x, resolution.x); ++x)
						{
							dvec3 rgb = dvec3(x, y, z) / dresMinusOne;
							dvec3 res = dvec3(0.0);
							double error = gaussNewton(rgb, res, GAUSS_NEWTON_ITERATION_COUNT, toXyz, item->toRGBLUT);
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

	dmat3x3 toMatrix(const double* valArray)
	{
		dmat3x3 mat;
		{
			const double* conv = valArray;
			double* dst = glm::value_ptr(mat);
			for (uint32_t i = 0; i < 9; ++i)
			{
				dst[i] = conv[i];
			}
			mat = glm::transpose(mat);

		}
		return mat;
	}

	dvec3 toCIELAB(dvec3 color, dmat3x3 toXyz)
	{
		constexpr float Xn = 95.047, Yn = 100.0, Zn = 108.883;
		constexpr float delta = 6.0 / 29.0;
		constexpr float delta3 = delta * delta * delta;

		dvec3 xyz = toXyz * color;

		dvec3 normalized = xyz / dvec3(Xn, Yn, Zn);
		dvec3 ft;

		for (uint32_t i = 0; i < 3; ++i)
		{
			ft[i] = normalized[i] > delta3 ? std::pow(normalized[i], (1.0 / 3.0)) : normalized[i] / (3.0 * delta * delta) + 4.0/ 29.0;
		}

		double L = 116.0 * ft.y - 16;
		double a = 500 * (ft.x - ft.y);
		double b = 200 * (ft.y - ft.z);

		return dvec3(L, a, b);
	}

	dvec3 calculateResiduals(dvec3 expected, dvec3 coeffs, const dmat3x3& toXYZ, const dvec3* toRGBLUT)
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
		for (size_t i = 0; i < CIE_LUT_RESOLUTION; ++i)
		{
			double lambda = double(i) / (CIE_LUT_RESOLUTION - 1);
			double polynomial = 0;
			for (uint32_t k = 0; k < 3; ++k)
			{
				polynomial = polynomial * lambda + coeffs[k];
			}

			double s = sigmoid(polynomial);

			estimated += s * toRGBLUT[i];
		}
		 //TODO: pbrt recommends transforming to cie lab colorspace before calculating residue 
		return toCIELAB(expected, toXYZ) - toCIELAB(estimated, toXYZ);
		
	}

	dmat3x3 calculateJacobian(dvec3 expected, dvec3 coeffs, const dmat3x3& toXYZ, const dvec3* toRGBLUT)
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
			residuals0 = calculateResiduals(expected, coeffs - delta, toXYZ, toRGBLUT);
			residuals1 = calculateResiduals(expected, coeffs + delta, toXYZ, toRGBLUT);

			jacobianOut[i] = (residuals1 - residuals0) * diffDenom;
		}
		return jacobianOut;
	}

	double gaussNewton(dvec3 expected, dvec3& coeffsInOut, size_t iterationCount, const dmat3x3& toXyz, const dvec3* toRGBLUT)
	{
		dvec3 coeffs = coeffsInOut;
		double sqrError = 0;
		const double ERROR_THRESHOLD = 1e-6;

		for (size_t i = 0; i < iterationCount; ++i)
		{
			dvec3 residuals = calculateResiduals(expected, coeffs, toXyz, toRGBLUT);
			dmat3x3 jacobian = calculateJacobian(expected, coeffs, toXyz, toRGBLUT);

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