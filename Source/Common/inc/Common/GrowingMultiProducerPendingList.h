#pragma once

#include "MultiProducerPendingList.h"
#include <vector>

#include <shared_mutex>
#include <algorithm>

namespace YAPT
{
	//like MultiProducerList but will dynamically grow when out of space (and incur lock overhead)
	template<typename T>
	class GrowingMultiProducerPendingList
	{
	public:

		GrowingMultiProducerPendingList(size_t initialSize);
		~GrowingMultiProducerPendingList();

		T* add(size_t count);
		T* getAll();

		size_t count() const;
		void clear();

	private:

		size_t getNextGrowAmount();
		
		std::shared_mutex m_mutex;
		MultiProducerPendingList<T> m_list;
	};

	template<typename T>
	GrowingMultiProducerPendingList<T>::GrowingMultiProducerPendingList(size_t initialSize)
		:m_list(initialSize)
	{

	}
	template<typename T>
	GrowingMultiProducerPendingList<T>::~GrowingMultiProducerPendingList()
	{

	}


	template<typename T>
	T* GrowingMultiProducerPendingList<T>::add(size_t count)
	{
		T* r = nullptr;
		

		while (r == nullptr)
		{
			size_t oldSize;
			{
				std::shared_lock<std::shared_mutex>(m_mutex);
				oldSize = m_list.count();
				r = m_list.add(count);
			}

			if (r == nullptr)
			{
				std::unique_lock<std::shared_mutex>(m_mutex);
				if (oldSize == m_list.count())
				{
					size_t extra = std::max(count, getNextGrowAmount());
					m_list.resize(m_list.count() + extra);
				}
			}
		}
		return r;
	}

	template<typename T>
	T* GrowingMultiProducerPendingList<T>::getAll()
	{
		return m_list.getAll();
	}
	template<typename T>
	size_t GrowingMultiProducerPendingList<T>::count() const
	{
		return m_list.count();
	}
	template<typename T>
	void GrowingMultiProducerPendingList<T>::clear()
	{
		m_list.clear();
	}

	template<typename T>
	size_t GrowingMultiProducerPendingList<T>::getNextGrowAmount()
	{
		return size_t(m_list.count() * 0.8);
	}

}