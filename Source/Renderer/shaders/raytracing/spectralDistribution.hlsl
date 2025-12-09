#ifndef SPECTRAL_DISTRIBUTION_HLSL_INCL
#define SPECTRAL_DISTRIBUTION_HLSL_INCL

#include "../common/colorSpaces.hlsl"
#include "../sharedIncludes/spectralConstants.h"

//the resources needed are defined in raytraceCommonResources.hlsl

#ifndef GET_SPECTRAL_SAMPLE_WAVELENGTH
#error "includer or spectralDistribution.hlsl needs to define GET_SPECTRAL_SAMPLE_WAVELENGTH which is the main wavelength to sample"
#endif

#define COLORSPACE_RGB 0
#define COLORSPACE_REC2020 1

#define COLORSPACE_DEFAULT COLORSPACE_RGB

#define SPECTRAL_SAMPLE_LAMBDA_RANGE (CIE_LUT_LAMBDA_MAX - CIE_LUT_LAMBDA_MIN)
#define SPECTRAL_SAMPLE_LAMBDA_PDF (1.f/SPECTRAL_SAMPLE_LAMBDA_RANGE)

float4 getSpectralSampleLambdas()
{
    const uint SAMPLE_COUNT = SPECTRAL_SAMPLES_COUNT;
    const float LAMBDA_RANGE = SPECTRAL_SAMPLE_LAMBDA_RANGE;
    float lambdaStep = LAMBDA_RANGE / SAMPLE_COUNT;
    float previousSample = GET_SPECTRAL_SAMPLE_WAVELENGTH;
    float4 res;

    res[0] = previousSample;

    for (uint i = 1; i < SAMPLE_COUNT; ++i)
    {
        float lambda = previousSample + lambdaStep;
        if (lambda > CIE_LUT_LAMBDA_MAX)
        {
            lambda = CIE_LUT_LAMBDA_MIN + (lambda - CIE_LUT_LAMBDA_MAX);
        }
        previousSample = lambda;
        res[i] = previousSample;
    }
    return res;
}

float calculateSpectralSampleWavelength(float rand)
{
    return CIE_LUT_LAMBDA_MIN + rand * SPECTRAL_SAMPLE_LAMBDA_RANGE;

}

float3 getXYZCoeffsForWavelength(float lambda)
{
	float u = (lambda - CIE_LUT_LAMBDA_MIN) / (CIE_LUT_LAMBDA_MAX - CIE_LUT_LAMBDA_MIN);
	return sampleLUT(g_lutSampler, g_cieXYZCoeffsLUT, u).xyz;
}

float getIlluminantCoeffForWavelength(float lambda)
{
	float u = (lambda - CIE_LUT_LAMBDA_MIN) / (CIE_LUT_LAMBDA_MAX - CIE_LUT_LAMBDA_MIN);
	return sampleLUT(g_lutSampler, g_d65IlluminantLUT, u).x;

}

float3 getRGBToSPDCoeffs(float3 color, int colorSpaceIndex = COLORSPACE_DEFAULT)
{
	if (colorSpaceIndex == COLORSPACE_RGB)
	{
		return sampleLUT(g_lutSampler, g_srgbToSPDLUT, color).xyz;
	}
	else if (colorSpaceIndex == COLORSPACE_REC2020)
	{
		return sampleLUT(g_lutSampler, g_rec2020ToSPDLUT, color).xyz;
	}
	else
	{
		return 0;
	}

}

float getHeroSpectralLambda()
{
    return GET_SPECTRAL_SAMPLE_WAVELENGTH;
}

float sigmoid(float x)
{
	if (isinf(x)) return x > 0 ? 1 : 0;
	return 0.5f * x / sqrt(1.0f + x * x) + 0.5f;
}

float sigmoidInv(float x)
{
	if (x == 1.0f) return 1.#INF;
	if (x == 0.0f) return -1.#INF;
	return (x - 0.5f) / sqrt(x * (1 - x));
}

struct SpectralSamples
{
	float samples[SPECTRAL_SAMPLES_COUNT];

	uint getSampleCount() { return SPECTRAL_SAMPLES_COUNT; }
	
    float4 toFloat4()
    {
        return float4(samples[0], samples[1], samples[2], samples[3]);

    }
	
    void fromFloat4(float4 v)
    {
        samples[0] = v.x;
        samples[1] = v.y;
        samples[2] = v.z;
        samples[3] = v.w;

    }

	float getMaxSampleValue() 
	{
		float v = samples[0];
		for (uint i = 1; i < SPECTRAL_SAMPLES_COUNT; ++i)
		{

			v = max(samples[i], v);
		}

		return v;
	}

	void setFromRGBUnbounded(float3 values)
	{
		float m = max(max(values.x, values.y), values.z);
		float scale = 2.f * m;
		setFromRGB(safeDiv(values, scale));

		for (uint i = 0; i < SPECTRAL_SAMPLES_COUNT; ++i)
		{
			
			samples[i] *= scale;
		}

		bool applyIlluminant = true;
		if (applyIlluminant)
		{
            float4 lambdas = getSpectralSampleLambdas();
			for (uint i = 0; i < SPECTRAL_SAMPLES_COUNT; ++i)
			{
                float waveLength = lambdas[i];
				samples[i] *= getIlluminantCoeffForWavelength(waveLength) * CIE_D65_SUM_INV * 100.f; //multiplied by 100 because currently all values become so low. Either need to think of different way to define inputs (what are we even inserting, there is no real measure) or find some more sensible value to normalize the distributions
			}
		}

	}


	void setFromRGB(float3 values)
	{
		float3 coeffs;
		if ((values.x == values.y) && (values.x == values.z))
		{
			coeffs = float3(0, 0, sigmoidInv(values.x));
		}
		else
		{
			coeffs = getRGBToSPDCoeffs(values);
		}
		setWithPolynomialCoeffs(coeffs);

		//TEST
		//samples[0] = values.x;
		//samples[1] = values.y;
		//samples[2] = values.z;

	}

	void setWithPolynomialCoeffs(float3 coeffs)
	{
        float4 lambdas = getSpectralSampleLambdas();

		for (uint i = 0; i < SPECTRAL_SAMPLES_COUNT; ++i)
		{
            float waveLength = lambdas[i];
			float lambda = (waveLength - CIE_LUT_LAMBDA_MIN) / (CIE_LUT_LAMBDA_MAX - CIE_LUT_LAMBDA_MIN); //TODO: change the coefficients to target actual values rather than the normalized [0,1] range, can get rid of this then
			float polynom = coeffs.x * lambda * lambda + coeffs.y * lambda + coeffs.z;
			float v = sigmoid(polynom);
			samples[i] = v;
		}
	}

	void set(float v)
	{
		for (int i = 0; i < SPECTRAL_SAMPLES_COUNT; ++i)
		{
			samples[i] = v;
		}
	}

	void setInd(uint ind, float v)
	{
		samples[ind] = v;
	}

	bool allSamplesEqual(float v)
	{
		for (uint i = 0; i < SPECTRAL_SAMPLES_COUNT; ++i)
		{
			if (samples[i] != v) return false;
		}
		return true;
	}

	SpectralSamples operator*(float v) 
	{
		SpectralSamples s;
		for (uint i = 0; i < SPECTRAL_SAMPLES_COUNT; ++i)
		{
			s.samples[i] = samples[i] * v;
		}
		return s;
	}

	SpectralSamples operator/(float v)
	{
		SpectralSamples s;
		for (uint i = 0; i < SPECTRAL_SAMPLES_COUNT; ++i)
		{
			s.samples[i] = samples[i] / v;
		}
		return s;
	}


	SpectralSamples operator+(float v)
	{
		SpectralSamples s;
		for (uint i = 0; i < SPECTRAL_SAMPLES_COUNT; ++i)
		{
			s.samples[i] = samples[i] + v;
		}
		return s;
	}

	SpectralSamples operator-(float v)
	{
		SpectralSamples s;
		for (uint i = 0; i < SPECTRAL_SAMPLES_COUNT; ++i)
		{
			s.samples[i] = samples[i] - v;
		}
		return s;
	}

	SpectralSamples operator*(SpectralSamples v)
	{
		SpectralSamples s;
		for (uint i = 0; i < SPECTRAL_SAMPLES_COUNT; ++i)
		{
			s.samples[i] = samples[i] * v[i];
		}
		return s;
	}

	SpectralSamples operator/(SpectralSamples v)
	{
		SpectralSamples s;
		for (uint i = 0; i < SPECTRAL_SAMPLES_COUNT; ++i)
		{
			s.samples[i] = samples[i] / v[i];
		}
		return s;
	}


	SpectralSamples operator+(SpectralSamples v)
	{
		SpectralSamples s;
		for (uint i = 0; i < SPECTRAL_SAMPLES_COUNT; ++i)
		{
			s.samples[i] = samples[i] + v[i];
		}
		return s;
	}

	SpectralSamples operator-(SpectralSamples v)
	{
		SpectralSamples s;
		for (uint i = 0; i < SPECTRAL_SAMPLES_COUNT; ++i)
		{
			s.samples[i] = samples[i] - v[i];
		}
		return s;
	}


	float operator[](uint x) {
		return samples[x];
	}

	void terminateSecondaryWavelengths()
	{
		for (uint i = 1; i < SPECTRAL_SAMPLES_COUNT; ++i)
		{
			samples[i] = 0;
		}

	}

	float3 ToXYZ(bool secondaryRaysTerminated = false)
	{

		float3 xyz = 0;

		if (secondaryRaysTerminated)
		{
			float waveLength = getHeroSpectralLambda();
			float pdf = SPECTRAL_SAMPLE_LAMBDA_PDF;
			float3 xyzCoeffs = getXYZCoeffsForWavelength(waveLength);
			xyz = samples[0] * xyzCoeffs * safeDiv(CIE_Y_SUM_INV, pdf);
			return xyz;
		}
		else
		{
			float4 lambdas = getSpectralSampleLambdas();
			for (uint i = 0; i < SPECTRAL_SAMPLES_COUNT; ++i)
			{

				float waveLength = lambdas[i];
				float pdf = SPECTRAL_SAMPLE_LAMBDA_PDF;
				float3 xyzCoeffs = getXYZCoeffsForWavelength(waveLength);
				xyz += samples[i] * xyzCoeffs * safeDiv(CIE_Y_SUM_INV, pdf);
			}
			return xyz / SPECTRAL_SAMPLES_COUNT;
		}

		
	}

	float3 ToRGB(int colorSpaceIndex = COLORSPACE_DEFAULT, bool secondaryRaysTerminated = false)
	{
		float3 xyz = ToXYZ(secondaryRaysTerminated);
		float3 rgb = 0;
		if (colorSpaceIndex == COLORSPACE_RGB)
		{
			rgb = mul(c_srgbXYZToRGB, xyz);
		}
		else if (colorSpaceIndex == COLORSPACE_REC2020)
		{
			rgb = mul(c_rec2020XYZToRGB, xyz);
		}

		return max(rgb, 0.f);

		//TEST
		//return float3(samples[0], samples[1], samples[2]);
	}

	bool hasNan()
	{
		for (uint i = 0; i < SPECTRAL_SAMPLES_COUNT; ++i)
		{
			if (isNan(samples[i])) return true;
		}
		return false;
	}
};

SpectralSamples sqrt(SpectralSamples v)
{
	SpectralSamples s;
	for (uint i = 0; i < SPECTRAL_SAMPLES_COUNT; ++i)
	{
		s.samples[i] = sqrt(v[i]);
	}
	return s;
}



#endif