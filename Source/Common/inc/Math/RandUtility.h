#pragma once
#include "Math.h"

namespace YAPT
{
	namespace MathUtils
	{
		constexpr uint32_t PRIME_NUMBERS[] =
		{
			2, 3, 5, 7, 11, 13, 17, 19, 23, 29,
			31, 37, 41, 43, 47, 53, 59, 61, 67, 71,
			73, 79, 83, 89, 97, 101, 103, 107, 109, 113,
			127, 131, 137, 139, 149, 151, 157, 163, 167, 173,
			179, 181, 191, 193, 197, 199, 211, 223, 227, 229,
			233, 239, 241, 251, 257, 263, 269, 271, 277, 281,
			283, 293, 307, 311, 313, 317, 331, 337, 347, 349,
			353, 359, 367, 373, 379, 383, 389, 397, 401, 409,
			419, 421, 431, 433, 439, 443, 449, 457, 461, 463,
			467, 479, 487, 491, 499, 503, 509, 521, 523, 541

		};

		inline uint32_t reverseBits32(uint32_t n) {
			n = (n << 16) | (n >> 16);
			n = ((n & 0x00ff00ff) << 8) | ((n & 0xff00ff00) >> 8);
			n = ((n & 0x0f0f0f0f) << 4) | ((n & 0xf0f0f0f0) >> 4);
			n = ((n & 0x33333333) << 2) | ((n & 0xcccccccc) >> 2);
			n = ((n & 0x55555555) << 1) | ((n & 0xaaaaaaaa) >> 1);
			return n;
		}  

		inline float vanDerCorputSequence(uint32_t index)
		{
			const float invMax = 1.0 / 0xFFFFFFFF;
			return float(reverseBits32(index)) * invMax;
		}


		inline vec2 hammersley(uint32_t i, uint32_t n)
		{
			return vec2(vanDerCorputSequence(i), float(i) / float(n));
		}

		template<typename RES_TYPE>
		RES_TYPE halton(size_t base, size_t index);

		template<typename RES_TYPE, glm::precision PRECISION, size_t... BASES>
		void generateHaltonSequence(size_t numberOfSamplesToGenerate, glm::vec<sizeof...(BASES), RES_TYPE, PRECISION>* samplesOut, size_t indexOffset);

		inline void generateHaltonSequence(size_t numberOfSamplesToGenerate, vec2* samplesOut, size_t indexOffset)
		{
			generateHaltonSequence<float, glm::precision::aligned_highp, PRIME_NUMBERS[0], PRIME_NUMBERS[1]>(numberOfSamplesToGenerate, samplesOut, indexOffset);
		}
		inline void generateHaltonSequence(size_t numberOfSamplesToGenerate, vec3* samplesOut, size_t indexOffset)
		{
			generateHaltonSequence<float, glm::precision::aligned_highp, PRIME_NUMBERS[0], PRIME_NUMBERS[1], PRIME_NUMBERS[2]>(numberOfSamplesToGenerate, samplesOut, indexOffset);
		}
		inline void generateHaltonSequence(size_t numberOfSamplesToGenerate, vec4* samplesOut, size_t indexOffset)
		{
			generateHaltonSequence<float, glm::precision::aligned_highp, PRIME_NUMBERS[0], PRIME_NUMBERS[1], PRIME_NUMBERS[2], PRIME_NUMBERS[3]>(numberOfSamplesToGenerate, samplesOut, indexOffset);
		}

		
		inline void generateHaltonSequence(size_t numberOfSamplesToGenerate, vec2p* samplesOut, size_t indexOffset)
		{
			generateHaltonSequence<float, glm::precision::packed_highp, PRIME_NUMBERS[0], PRIME_NUMBERS[1]>(numberOfSamplesToGenerate, samplesOut, indexOffset);
		}
		inline void generateHaltonSequence(size_t numberOfSamplesToGenerate, vec3p* samplesOut, size_t indexOffset)
		{
			generateHaltonSequence<float, glm::precision::packed_highp, PRIME_NUMBERS[0], PRIME_NUMBERS[1], PRIME_NUMBERS[2]>(numberOfSamplesToGenerate, samplesOut, indexOffset);
		}
		inline void generateHaltonSequence(size_t numberOfSamplesToGenerate, vec4p* samplesOut, size_t indexOffset)
		{
			generateHaltonSequence<float, glm::precision::packed_highp, PRIME_NUMBERS[0], PRIME_NUMBERS[1], PRIME_NUMBERS[2], PRIME_NUMBERS[3]>(numberOfSamplesToGenerate, samplesOut, indexOffset);
		}
		
	}
}

#include "RandUtility.inl"

