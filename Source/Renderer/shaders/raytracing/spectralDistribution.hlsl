#ifndef SPECTRAL_DISTRIBUTION_HLSL_INCL
#define SPECTRAL_DISTRIBUTION_HLSL_INCL

//the resources needed are defined in raytraceCommonResources.hlsl

float3 getXYZCoeffsForWavelength(float lambda)
{
	float u = (lambda - CIE_LUT_LAMBDA_MIN) / (CIE_LUT_LAMBDA_MAX - CIE_LUT_LAMBDA_MIN);
	return sampleLUT(g_lutSampler, g_cieXYZCoeffsLUT, u).xyz;
}

struct SpectralSamples
{
	float samples[SPECTRAL_SAMPLES_COUNT];

	uint getSampleCount() { return SPECTRAL_SAMPLES_COUNT; }

	void setFromXYZ(float3 values)
	{
		for (uint i = 0; i < SPECTRAL_SAMPLES_COUNT; ++i)
		{
			samples[i] = values[i];
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
		/*float3 xyz = 0;
		for (uint i = 0; i < SPECTRAL_SAMPLES_COUNT; ++i)
		{
			float2 lambdaPDF = g_sampledWavelengthAndPDF[i];
			float3 xyzCoeffs = getXYZCoeffsForWavelength(lambdaPDF.x);
			xyz += samples[i] * safeDiv(xyzCoeffs, lambdaPDF.y);
		}
		return xyz / SIE_Y_SUM;*/

		return float3(samples[0], samples[1], samples[2]);
	}

	float3 ToRGB()
	{
		float3 xyz = ToXYZ();
		return xyz;
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