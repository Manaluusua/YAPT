#pragma once

#include <atomic>
#include <vector>

namespace YAPT
{
	//Only addition is thread safe
	template<typename T>
	class MultiProducerPendingList
	{
	public:
		MultiProducerPendingList(size_t size);
		~MultiProducerPendingList();

		
		T* add(size_t count);
		T* getAll();

		void resize(size_t count);

		size_t count() const;
		void clear();

	private:

		std::vector<T> m_entries;
		std::atomic<size_t> m_used;
	};

	template<typename T>
	MultiProducerPendingList<T>::MultiProducerPendingList(size_t size)
	{
		resize(size);
		m_used.store(0);
	}

	template<typename T>
	MultiProducerPendingList<T>::~MultiProducerPendingList()
	{

	}

	template<typename T>
	void MultiProducerPendingList<T>::resize(size_t count)
	{
		m_entries.resize(count);
	}

	template<typename T>
	T* MultiProducerPendingList<T>::add(size_t count)
	{
		T* ret = nullptr;
		while (true)
		{
			size_t used = m_used.load();
			if ((used + count) < m_entries.size())
			{
				if (m_used.compare_exchange_strong(used, used + count))
				{
					ret = &m_entries[used];
					break;
				}
			}
			else
			{
				break;
			}
		}
		return ret;
	}

	template<typename T>
	T* MultiProducerPendingList<T>::getAll()
	{
		return m_entries.data();
	}
	template<typename T>
	size_t MultiProducerPendingList<T>::count() const
	{
		return m_used.load();
	}
	template<typename T>
	void MultiProducerPendingList<T>::clear()
	{
		m_used.store(0);
	}

}