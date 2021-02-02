#pragma once
#include "Math.h"

namespace YAPT
{
	namespace MathUtils
	{
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
			generateHaltonSequence<float, glm::precision::aligned_highp, 2, 3>(numberOfSamplesToGenerate, samplesOut, indexOffset);
		}
		inline void generateHaltonSequence(size_t numberOfSamplesToGenerate, vec3* samplesOut, size_t indexOffset)
		{
			generateHaltonSequence<float, glm::precision::aligned_highp, 2, 3, 5>(numberOfSamplesToGenerate, samplesOut, indexOffset);
		}
		inline void generateHaltonSequence(size_t numberOfSamplesToGenerate, vec4* samplesOut, size_t indexOffset)
		{
			generateHaltonSequence<float, glm::precision::aligned_highp, 2, 3, 5, 7>(numberOfSamplesToGenerate, samplesOut, indexOffset);
		}

		
		inline void generateHaltonSequence(size_t numberOfSamplesToGenerate, vec2p* samplesOut, size_t indexOffset)
		{
			generateHaltonSequence<float, glm::precision::packed_highp, 2, 3>(numberOfSamplesToGenerate, samplesOut, indexOffset);
		}
		inline void generateHaltonSequence(size_t numberOfSamplesToGenerate, vec3p* samplesOut, size_t indexOffset)
		{
			generateHaltonSequence<float, glm::precision::packed_highp, 2, 3, 5>(numberOfSamplesToGenerate, samplesOut, indexOffset);
		}
		inline void generateHaltonSequence(size_t numberOfSamplesToGenerate, vec4p* samplesOut, size_t indexOffset)
		{
			generateHaltonSequence<float, glm::precision::packed_highp, 2, 3, 5, 7>(numberOfSamplesToGenerate, samplesOut, indexOffset);
		}
		
	}
}

#include "RandUtility.inl"

