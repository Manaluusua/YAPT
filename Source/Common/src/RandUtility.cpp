#include <Math/RandUtility.h>
#include <vector>
namespace YAPT
{
	namespace MathUtils
	{
		class ScrambledPrimesUtility
		{
		public:

			ScrambledPrimesUtility()
			{
				m_primePrefixSum.resize(PRIME_NUMBERS_COUNT);

				uint32_t scrambledIndicesCount = 0;
				for (int i = 0; i < PRIME_NUMBERS_COUNT; ++i)
				{
					m_primePrefixSum[i] = scrambledIndicesCount;
					scrambledIndicesCount += PRIME_NUMBERS[i];
				}

				m_scrambledIndices.resize(scrambledIndicesCount);

				uint16_t* p = m_scrambledIndices.data();
				for (uint16_t i = 0; i < PRIME_NUMBERS_COUNT; ++i)
				{
					for (uint16_t j = 0; j < PRIME_NUMBERS[i]; ++j)
					{
						p[j] = j;
					}
						
					shuffleArray(p, PRIME_NUMBERS[i]);
					p += PRIME_NUMBERS[i];
				}
			}

			~ScrambledPrimesUtility()
			{

			}

			const uint16_t* getScrambledIndicesForPrimeIndex(uint32_t index)
			{
				assert(index < PRIME_NUMBERS_COUNT);
				return &m_scrambledIndices[m_primePrefixSum[index]];
			}


		private:
			std::vector<uint32_t> m_primePrefixSum;
			std::vector<uint16_t> m_scrambledIndices;
		};

		static ScrambledPrimesUtility s_scrambledPrimesUtility;


		const uint16_t* getScrambledDigitsForPrimeIndex(uint32_t index)
		{
			return s_scrambledPrimesUtility.getScrambledIndicesForPrimeIndex(index);
		}
	}

}