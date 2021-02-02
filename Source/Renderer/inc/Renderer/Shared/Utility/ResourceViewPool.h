#pragma once

#include <unordered_map>
#include <shared_mutex>
#include <atomic>
#include <xxhash.h>

namespace YAPT
{
	template<typename DESCRIPTION, typename VIEWINSTANCE, typename VIEWINSTANCEFACTORY, size_t EARLYENTRYCOUNT>
	class ResourceViewPool
	{
	public:

		ResourceViewPool(VIEWINSTANCEFACTORY& factory)
			:m_viewInstanceFactory(factory),
			m_currentEarlyEntryCountRead(0),
			m_currentEarlyEntryCountWrite(0)
		{

		}
		~ResourceViewPool()
		{
			for (size_t i = 0; i < EARLYENTRYCOUNT; ++i)
			{
				if (m_earlyEntries[i].valid)
				{
					m_viewInstanceFactory.destroyView(m_earlyEntries[i].instance);
				}
				
			}

			for (auto iter = m_views.begin(); iter != m_views.end(); ++iter)
			{
				m_viewInstanceFactory.destroyView(iter->second);
			}

		}

		VIEWINSTANCE get(const DESCRIPTION& desc)
		{
			
			while (true)
			{
				size_t earlyEntriesCountRead = m_currentEarlyEntryCountRead.load();

				for (size_t i = 0; i < earlyEntriesCountRead; ++i)
				{
					if (desc == m_earlyEntries[i].desc)
					{
						return m_earlyEntries[i].instance;
					}
				}

				size_t earlyEntriesCountWrite = m_currentEarlyEntryCountWrite.load();

				if (earlyEntriesCountWrite < EARLYENTRYCOUNT)
				{
					if (!m_currentEarlyEntryCountWrite.compare_exchange_strong(earlyEntriesCountWrite, earlyEntriesCountWrite + 1))
					{
						continue;
					}

					m_earlyEntries[earlyEntriesCountWrite].desc = desc;
					m_earlyEntries[earlyEntriesCountWrite].instance = m_viewInstanceFactory.createView(desc);
					m_earlyEntries[earlyEntriesCountWrite].valid = true;

					m_currentEarlyEntryCountRead.fetch_add(1);

					return m_earlyEntries[earlyEntriesCountWrite].instance;
				}
				else
				{
					break;
				}
			}
			
			//no valid view found in early entries, search hashmap
			{
				std::shared_lock<std::shared_mutex> lock(m_mutex);
				auto iter = m_views.find(desc);
				if (iter != m_views.end())
				{
					return iter->second;
				}
			}
			
			std::unique_lock<std::shared_mutex> lock(m_mutex);
			auto iter = m_views.find(desc);
			if (iter != m_views.end())
			{
				return iter->second;
			}
			else
			{
				VIEWINSTANCE instance = m_viewInstanceFactory.createView(desc);
				m_views[desc] = instance;
				return instance;
			}

		}

	private:

		struct DefaultEntries
		{
			DESCRIPTION desc;
			VIEWINSTANCE instance;
			bool valid = false;
		};

		struct Hasher
		{
			std::size_t operator()(const DESCRIPTION& s) const
			{
				return XXH64(&s, sizeof(s), 0);
			}
		};

		DefaultEntries m_earlyEntries[EARLYENTRYCOUNT];
		std::atomic<size_t> m_currentEarlyEntryCountRead;
		std::atomic<size_t> m_currentEarlyEntryCountWrite;

		VIEWINSTANCEFACTORY& m_viewInstanceFactory;

		std::shared_mutex m_mutex;
		std::unordered_map<DESCRIPTION, VIEWINSTANCE, Hasher> m_views;
	};

}