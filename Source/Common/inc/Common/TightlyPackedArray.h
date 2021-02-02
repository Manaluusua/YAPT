#pragma once
#include <vector>
#include <assert.h>
#include <Common/StructureOfArrays.h>

namespace YAPT
{
	/**
	* SOA -type of container which also keeps the internal structure in memory without gaps using swap-erase.
	* This means that the id returned is an identifier for the entry, not an index to the memory returned by getData().
	*/

	typedef size_t TPAID;
	constexpr size_t InvalidTPAID = -1;

	template< class ... TYPES >
	class TightlyPackedArray
	{
	public:

		const static size_t STRUCTURE_COUNT = sizeof...(TYPES);

		TightlyPackedArray(size_t initialReserve = 0);

		template< typename ... PARAMS >
		TPAID addEntry(PARAMS&&... params);
		void removeEntry(TPAID id);

		template<size_t dataIndex>
		auto& getDataEntryWithId(TPAID id);

		template<size_t dataIndex>
		const auto& getDataEntryWithId(TPAID id) const;

		template<size_t dataIndex>
		auto getData() const;

		template<size_t dataIndex>
		auto getData();

		size_t getDataIndex(TPAID id) const;
		size_t getDataCount() const;

		TPAID getHighestAllocatedId() const { return m_nextId == 0 ? 0 : m_nextId - 1; }

	private:
		StructureOfArrays<TYPES...> m_data;
		std::vector<size_t> m_idToDataIndexMapping;
		std::vector<TPAID> m_dataIndexToIdMapping;
		std::vector<TPAID> m_freeIds;
		TPAID m_nextId;

	};

	template<typename ... TYPES>
	TightlyPackedArray<TYPES...>::TightlyPackedArray(size_t initialReserve)
		:m_nextId(0)
	{
		if (initialReserve > 0)
		{

			m_data.reserve(initialReserve);
			m_idToDataIndexMapping.reserve(initialReserve);
			m_dataIndexToIdMapping.reserve(initialReserve);
		}
		

	}
	template< typename ... TYPES >
	template< typename ... PARAMS >
	TPAID TightlyPackedArray<TYPES...>::addEntry(PARAMS&&... params)
	{
		TPAID id;
		if (m_freeIds.size() > 0)
		{
			id = m_freeIds.back();
			m_freeIds.pop_back();
		}
		else
		{
			id = m_nextId++;
			m_idToDataIndexMapping.resize(m_nextId);
		}

		size_t dataIndex = m_data.size();
		m_data.emplace_back(std::forward<PARAMS>(params)...);

		m_dataIndexToIdMapping.push_back(id);
		m_idToDataIndexMapping[id] = dataIndex;

		return id;
	}

	template< typename ... TYPES >
	void TightlyPackedArray<TYPES...>::removeEntry(TPAID entry)
	{
		assert(entry < m_idToDataIndexMapping.size());
		assert(m_data.size() > 0);

		size_t dataIndexToRemove = m_idToDataIndexMapping[entry];

		size_t tailId = m_dataIndexToIdMapping.back();
		size_t tailIndex = m_dataIndexToIdMapping.size() - 1;

		std::swap(m_idToDataIndexMapping[entry], m_idToDataIndexMapping[tailId]);

		m_data.swap(dataIndexToRemove, tailIndex);
		std::swap(m_dataIndexToIdMapping[dataIndexToRemove], m_dataIndexToIdMapping[tailIndex]);

		m_data.pop_back();
		m_dataIndexToIdMapping.pop_back();


		m_freeIds.push_back(entry);

	}

	template< typename ... TYPES >
	size_t TightlyPackedArray<TYPES...>::getDataIndex(TPAID id) const
	{
		assert(id < m_idToDataIndexMapping.size());
		return m_idToDataIndexMapping[id];
	}

	template< typename ... TYPES >
	template<size_t dataIndex>
	auto& TightlyPackedArray<TYPES...>::getDataEntryWithId(TPAID id)
	{
		auto dataPtr = getData<dataIndex>();
		return dataPtr[getDataIndex(id)];
	}

	template< typename ... TYPES>
	template<size_t dataIndex>
	auto TightlyPackedArray<TYPES...>::getData()
	{
		return m_data.getArrayPtr<dataIndex>();
	}

	template< typename ... TYPES >
	template<size_t dataIndex>
	const auto& TightlyPackedArray<TYPES...>::getDataEntryWithId(TPAID id) const
	{
		const auto* dataPtr = getData<dataIndex>();
		return dataPtr[getDataIndex(id)];
	}
	
	template< typename ... TYPES>
	template<size_t dataIndex>
	auto TightlyPackedArray<TYPES...>::getData() const
	{
		return m_data.getArrayPtr<dataIndex>();
	}



	template< typename ... TYPES >
	size_t TightlyPackedArray<TYPES...>::getDataCount() const
	{
		return m_data.size();
	}


}
