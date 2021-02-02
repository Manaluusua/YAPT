#pragma once

#include <vector>
#include <assert.h>

namespace YAPT
{
	template<size_t INDEX, class T>
	struct GetImpl;

	template<class... T>
	struct StructureOfArrays
	{
		void reserve(size_t size)
		{
		}

		void emplace_back()
		{
		}

		size_t size() const
		{
		}

		void swap(size_t a, size_t b)
		{
		}

		void pop_back()
		{

		}
	};


	template<class T, class... OTHERS>
	struct StructureOfArrays<T, OTHERS...>
	{
	public:
		

		void reserve(size_t size)
		{
			current.reserve(size);
			others.reserve(size);
		}

		template<class PARAM1, class...PARAMSREST>
		void emplace_back(PARAM1&& a, PARAMSREST&&... b)
		{
			current.emplace_back(a);
			others.emplace_back(b...);
		}


		void pop_back()
		{
			current.pop_back();
			others.pop_back();
		}

		size_t size() const
		{
			return current.size();
		}

		void swap(size_t a, size_t b)
		{
			std::swap(current[a], current[b]);
			others.swap(a, b);
		}

		template<size_t INDEX>
		auto getArrayPtr()
		{
			return GetImpl<INDEX, StructureOfArrays<T, OTHERS...>>::getEntry(*this);
		}

	
		std::vector<T> current;
		StructureOfArrays<OTHERS...> others;
	};


	template<typename T, typename ... OTHERS>
	struct GetImpl<0, StructureOfArrays<T, OTHERS ... >>
	{
		static T* getEntry(StructureOfArrays<T, OTHERS...>& data)
		{
			return data.current.data();
		}
	};

	template<size_t INDEX, typename T, typename ... OTHERS>
	struct GetImpl<INDEX, StructureOfArrays<T, OTHERS ... >>
	{
		static auto getEntry(StructureOfArrays<T, OTHERS...>& data)
		{
			return GetImpl<INDEX - 1, StructureOfArrays<OTHERS...>>::getEntry(data.others);
		}
	};

}
