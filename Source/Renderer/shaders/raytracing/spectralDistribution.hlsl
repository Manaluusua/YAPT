#ifndef SPECTRAL_DISTRIBUTION_HLSL_INCL
#define SPECTRAL_DISTRIBUTION_HLSL_INCL

#include "../utils/colorSpaces.hlsl"

//the resources needed are defined in raytraceCommonResources.hlsl

float3 getXYZCoeffsForWavelength(float lambda)
{
	float u = (lambda - CIE_LUT_LAMBDA_MIN) / (CIE_LUT_LAMBDA_MAX - CIE_LUT_LAMBDA_MIN);
	return sampleLUT(g_lutSampler, g_cieXYZCoeffsLUT, u).xyz;
}

float3 getRGBToSPDCoeffs(float3 color)
{
	return sampleLUT(g_lutSampler, g_srgbToSPDLUT, color).xyz;
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

	void setFromRGBUnbounded(float3 values)
	{
		float m = max(max(values.x, values.y), values.z);
		float scale = 2.f * m;
		setFromRGB(safeDiv(values, scale));

		for (uint i = 0; i < SPECTRAL_SAMPLES_COUNT; ++i)
		{
			samples[i] *= scale;
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
		/*samples[0] = values.x;
		samples[1] = values.y;
		samples[2] = values.z;*/

	}

	void setWithPolynomialCoeffs(float3 coeffs)
	{
		uint sampleSetIndex = getSpectralSampleSetIndex();

		for (uint i = 0; i < SPECTRAL_SAMPLES_COUNT; ++i)
		{
			uint sampleIndex = sampleSetIndex * SPECTRAL_SAMPLES_COUNT + i;

			float waveLength = getSpectralSampleLambda(sampleIndex);
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

	float3 ToXYZ()
	{
		uint sampleSetIndex = getSpectralSampleSetIndex();

		float3 xyz = 0;
		for (uint i = 0; i < SPECTRAL_SAMPLES_COUNT; ++i)
		{
			uint sampleIndex = sampleSetIndex * SPECTRAL_SAMPLES_COUNT + i;

			float waveLength = getSpectralSampleLambda(sampleIndex);
			float pdf = getSpectralSampleLambdaPDF(sampleIndex);
			float3 xyzCoeffs = getXYZCoeffsForWavelength(waveLength);
			xyz += samples[i] * xyzCoeffs * safeDiv(CIE_Y_SUM_INV, pdf) ;
		}
		return xyz / SPECTRAL_SAMPLES_COUNT;
	}

	float3 ToRGB()
	{
		float3 xyz = ToXYZ();
		return mul(c_srgbXYZToRGB, xyz);

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