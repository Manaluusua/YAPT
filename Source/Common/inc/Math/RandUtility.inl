#include <glm/glm.hpp>
#include <array>

#include "assert.h"

namespace YAPT
{
	namespace MathUtils
	{
		//from pbrt
		inline float halton(uint64_t base, uint64_t index, const uint16_t* scrambledDigits)
		{
			// We have to stop once reversedDigits is >= limit since otherwise the
			// next digit of |a| may cause reversedDigits to overflow.
			uint64_t limit = ~0ull / base - base;
			float invBase = 1.f / (float)base;
			float invBaseM = 1.f;
			uint64_t reversedDigits = 0;
			int digitIndex = 0;
			while (1 - (base - 1) * invBaseM < 1 && reversedDigits < limit) 
			{
				// Permute least significant digit from _a_ and update _reversedDigits_
				uint64_t next = index / base;
				uint64_t digitValue = index - next * base;
				reversedDigits = reversedDigits * base + scrambledDigits[digitValue];
				invBaseM *= invBase;
				++digitIndex;
				index = next;
			}
			return glm::min(invBaseM * reversedDigits, s_oneMinusEpsilonFloat);
		}

		inline uint32_t sobolUint(uint32_t index, uint32_t dimension)
		{
			uint32_t val = 0;
			assert(dimension < SOBOL_MATRIX_COUNT && "Tried to ask for a sobol sequence with a dimension going above the limit");
			for (uint32_t i = dimension * SOBOL_MATRIX_ROW_COUNT; index != 0; index >>= 1, ++i)
			{
				if (index & 1)
				{
					val ^= SOBOL_MATRICES[i];
				}
			}
			return val;
		}

		inline float sobolFloat(uint32_t index, uint32_t dimension)
		{
			uint32_t v = sobolUint(index, dimension);
			return v * 0x1p-32f;
		}

		template<typename RES_TYPE, glm::precision PRECISION, size_t... PRIMEINDICES>
		void generateHaltonSequence(size_t numberOfSamplesToGenerate, glm::vec<sizeof...(PRIMEINDICES), RES_TYPE, PRECISION>* samplesOut, size_t indexOffset)
		{
			constexpr size_t numberOfDimensions = sizeof...(PRIMEINDICES);
			std::array<size_t, numberOfDimensions> primeIndices{ PRIMEINDICES...};

			for (size_t i = 0; i < numberOfSamplesToGenerate; ++i)
			{
				glm::vec<numberOfDimensions, RES_TYPE, PRECISION> sample;
				for (size_t componentIndex = 0; componentIndex < numberOfDimensions; ++componentIndex)
				{
					size_t primeIndex = primeIndices[componentIndex];
					sample[static_cast<typename glm::vec<sizeof...(PRIMEINDICES), RES_TYPE, PRECISION>::length_type>(componentIndex)] = halton(PRIME_NUMBERS[primeIndex], i + indexOffset, getScrambledDigitsForPrimeIndex((uint32_t)primeIndex));
				}
				samplesOut[i] = sample;
			}

		}

		template<typename RES_TYPE>
		void generateHaltonSequenceWithDimensions(size_t numberOfSamplesToGenerate, uint32_t numberOfComponents, RES_TYPE* samplesOut, size_t indexOffset)
		{
			size_t sampleIndex = 0;
			for (size_t i = 0; i < numberOfSamplesToGenerate; ++i)
			{
				for (uint32_t k = 0 ; k < numberOfComponents; ++k)
				{
					samplesOut[sampleIndex++] = halton(PRIME_NUMBERS[k], i + indexOffset, getScrambledDigitsForPrimeIndex(k));
				}
			}

		}


		
		inline void generateSobolSequenceWithDimensions(uint32_t numberOfSamplesToGenerate, uint32_t numberOfComponents, float* samplesOut, uint32_t indexOffset)
		{
			size_t sampleIndex = 0;
			for (uint32_t i = 0; i < numberOfSamplesToGenerate; ++i)
			{
				for (uint32_t k = 0; k < numberOfComponents; ++k)
				{
					samplesOut[sampleIndex++] = sobolFloat(i + indexOffset, k);
				}
			}
		}

		inline void generateSobolSequenceWithDimensions(uint32_t numberOfSamplesToGenerate, uint32_t numberOfComponents, uint32_t* samplesOut, uint32_t indexOffset)
		{
			size_t sampleIndex = 0;
			for (uint32_t i = 0; i < numberOfSamplesToGenerate; ++i)
			{
				for (uint32_t k = 0; k < numberOfComponents; ++k)
				{
					samplesOut[sampleIndex++] = sobolUint(i + indexOffset, k);
				}
			}
		}

	}
}