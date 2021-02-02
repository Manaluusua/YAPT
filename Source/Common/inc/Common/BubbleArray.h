#pragma once

#include <vector>
#include <assert.h>

namespace YAPT
{
	/**
	* Array which maintains indices when entries are removed, thus leaving "bubbles" or empty entries. Removed index is re-filled when adding new entries.
	* Id is the index in the data array
	*/
	constexpr size_t InvalidBubbleArrayIndex = -1;

	template< typename T >
	class BubbleArray
	{
	public:

		struct DataEntry
		{
			template<typename ...PARAMS>
			DataEntry(PARAMS... params)
				:data(params...),
				entryValid(true)
			{}

			T data;
			bool entryValid;
		};

		template<typename U, typename V>
		class BubbleArrayIterator
		{
			friend class BubbleArray<T>;
		public:

			BubbleArrayIterator(const BubbleArrayIterator<U, V>& o)
				:m_current(o.m_current),
				m_begin(o.m_begin)
				m_end(o.m_end)
			{

			}

			BubbleArrayIterator(const BubbleArrayIterator<U, V>&& o)
				:m_current(o.m_current),
				m_begin(o.m_begin)
				m_end(o.m_end)
			{

			}

			V getNextValidEntry()
			{
				V val = nullptr;

				if (m_current != m_end)
				{
					++m_current;
					while (m_current != m_end)
					{
						if (m_current->entryValid)
						{
							val = &m_current->data;
							break;
						}
						++m_current;
					}
					
					
				}

				return val;
			}

			V getCurrent() const
			{
				V val = nullptr;
				if (m_current != m_end)
				{
					if (m_current->entryValid)
					{
						val = &m_current->data;
						
					}
				}
				return val;
			}

		private:

			void initialize()
			{
				while (m_current != m_end)
				{
					if (m_current->entryValid)
					{
						return;
					}
					++m_current;
				}
			}

			BubbleArrayIterator(U d, U end)
				:m_current(d),
				m_end(end)
			{
				initialize();
			}
			
			U m_current;
			U m_end;
		};

		typedef BubbleArrayIterator<DataEntry*, T*> Iterator;
		typedef BubbleArrayIterator<const DataEntry*, const T*> ConstIterator;


		BubbleArray(size_t initialReserve = 0);

		template<typename... Params>
		size_t addEntry(Params&&... params);
		void removeEntry(size_t id);

		T& getDataEntryWithId(size_t id);
		const T& getDataEntryWithId(size_t id) const;

		size_t getNumberOfActiveEntries() const { return m_data.size() - m_freeIds.size(); }
		size_t getHighestIndexAllocated() const { return m_nextId == 0 ? m_nextId : m_nextId - 1; }

		Iterator getIterator()
		{
			DataEntry* ptr = m_data.data();
			return Iterator(ptr, ptr + m_data.size());
		}

		ConstIterator getConstIterator() const
		{
			const DataEntry* ptr = m_data.data();
			return ConstIterator(ptr, ptr + m_data.size());
		}

	private:
		std::vector<DataEntry> m_data;
		std::vector<size_t> m_freeIds;
		size_t m_nextId;

	};

	template<typename T> 
	BubbleArray<T>::BubbleArray(size_t initialReserve)
		:m_nextId(0)
	{
		if (initialReserve > 0)
		{
			m_data.reserve(initialReserve);
		}
		

	}
	
	template<typename T>
	template<typename... Params>
	size_t BubbleArray<T>::addEntry(Params&&... params)
	{
		size_t id;
		if (m_freeIds.size() > 0)
		{
			id = m_freeIds.back();
			m_freeIds.pop_back();

			new (&m_data[id].data) T(id, (params)...);
		}
		else
		{
			id = m_nextId++;
			m_data.emplace_back(id, std::forward<Params>(params)...);
		}
		return id;
	}

	template<typename T>
	void BubbleArray<T>::removeEntry(size_t entry)
	{
		assert(m_data.size() > entry);
		m_freeIds.push_back(entry);
		m_data[entry].entryValid = false;
	}



	template<typename T>
	T& BubbleArray<T>::getDataEntryWithId(size_t id)
	{
		assert(id < m_data.size());
		return m_data[id].data;
	}

	template<typename T>
	const T& BubbleArray<T>::getDataEntryWithId(size_t id) const
	{
		assert(id < m_data.size());
		return m_data[id].data;
	}


}
