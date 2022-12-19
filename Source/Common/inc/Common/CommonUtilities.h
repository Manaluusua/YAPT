#pragma once
#include <stddef.h>

#define YAPT_NOCOPY(TYPE) \
	TYPE(const TYPE&) = delete; \
	TYPE& operator=(const TYPE&) = delete;


namespace YAPT
{
	template<typename T>
	constexpr T max(const T& a, const T& b)
	{
		return a >= b ? a : b;
	}

	template<typename T>
	constexpr T min(const T& a, const T& b)
	{
		return a <= b ? a : b;
	}


	template<typename T>
	constexpr T clamp(const T& x, const T& minimum, const T& maximum)
	{
		return min(max(x, minimum), maximum);
	}

	template<typename T, typename U> 
	constexpr T align(T val, U alignment)
	{
		T mask = T(alignment - 1);
		return (val + mask)&~(mask);
	}

	template<typename T, typename U>
	constexpr void* align(T*& ptr, U alignment)
	{
		intptr_t ptrVal = reinterpret_cast<intptr_t>(ptr);
		ptrVal = align(ptrVal, alignment);
		
		return reinterpret_cast<void*>(ptrVal);
	}


	template<typename T>
	constexpr bool isAligned(T val, T alignment)
	{
		return val == align(val, alignment);
	}

	template< class Type, ptrdiff_t n >
	constexpr ptrdiff_t countOf(Type(&)[n]) { return n; }

	
	constexpr uint32_t getLSB(uint32_t value)
	{
		constexpr uint32_t MultiplyDeBruijnBitPosition[32] =
		{
		  0, 1, 28, 2, 29, 14, 24, 3, 30, 22, 20, 15, 25, 17, 4, 8,
		  31, 27, 13, 23, 21, 19, 16, 7, 26, 12, 18, 6, 11, 5, 10, 9
		};
		return MultiplyDeBruijnBitPosition[((uint32_t)((value & -int32_t(value)) * 0x077CB531U)) >> 27];
	}
}

