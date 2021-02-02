
#include <array>

namespace YAPT
{
	namespace MathUtils
	{
		template<typename RES_TYPE>
		RES_TYPE halton(size_t base, size_t index)
		{
			RES_TYPE res = 0;
			RES_TYPE f = 1;

			while (index > 0)
			{
				f = f / base;
				res += f * (index % base);
				index = index / base;
			}
			return res;
		}

		template<typename RES_TYPE, glm::precision PRECISION, size_t... BASES>
		void generateHaltonSequence(size_t numberOfSamplesToGenerate, glm::vec<sizeof...(BASES), RES_TYPE, PRECISION>* samplesOut, size_t indexOffset)
		{
			constexpr size_t numberOfDimensions = sizeof...(BASES);
			std::array<size_t, numberOfDimensions> bases{BASES...};

			for (size_t i = 0; i < numberOfSamplesToGenerate; ++i)
			{
				glm::vec<numberOfDimensions, RES_TYPE, PRECISION> sample;
				for (size_t componentIndex = 0; componentIndex < numberOfDimensions; ++componentIndex)
				{

					sample[static_cast<typename glm::vec<sizeof...(BASES), RES_TYPE, PRECISION>::length_type>(componentIndex)] = halton<RES_TYPE>(bases[componentIndex], i + indexOffset);
				}
				samplesOut[i] = sample;
			}

		}



	}
}