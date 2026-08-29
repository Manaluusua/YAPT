#pragma once

#include <Renderer/ReadbackHandle.h>
#include <Common/DeferredFreeArrayIndexAllocator.h>
#include <atomic>

namespace YAPT
{
	class CRenderer;
	class ReadbackManager;

	class CReadbackObject : public ReadbackObject
	{
	public:
		friend class ReadbackManager;
		CReadbackObject()
			:m_state(ReadbackState::Freed),
			m_index(uint32_t(-1))
		{

		}

		virtual ReadbackState getState() final
		{
			return m_state.load();
		}

		virtual void allReferencesReleased() final
		{
			setState(ReadbackState::Freed);
		}
	private:

		virtual ReadbackState getStateNonAtomic() final
		{
			return m_state;
		}

		void setReadbackTarget(ReadbackTarget target)
		{
			m_target = target;
		}

		ReadbackTarget getReadbackTarget() const
		{
			return m_target;
		}

		void setState(ReadbackState state)
		{
			if (state == ReadbackState::Ready)
			{
				while (m_state.load() == ReadbackState::Pending)
				{
					ReadbackState expected = ReadbackState::Pending;
					if (m_state.compare_exchange_weak(expected, ReadbackState::Ready, std::memory_order_acq_rel))
					{
						break;
					}
				}
			}
			else
			{
				m_state.store(state);
			}
			
		}

		void setIndex(uint32_t index)
		{
			m_index = index;
		}
		uint32_t getIndex() const
		{
			return m_index;
		}
		
		std::atomic<ReadbackState> m_state;
		uint32_t m_index;
		ReadbackTarget m_target;
	};

	class ReadbackManager 
	{
		friend class CReadbackObject;
	public:
		ReadbackManager(CRenderer& renderer);
		~ReadbackManager();

		void commitChanges();

		ReadbackHandle readback(ReadbackTarget target);
	private:
		static constexpr uint32_t MAX_READBACK_REQUESTS = 256;

		void freeResource(uint32_t index, ReadbackTarget target);
		void allocateResource(uint32_t index, ReadbackTarget target);

		CRenderer& m_renderer;
		CReadbackObject m_readbackObjectPool[MAX_READBACK_REQUESTS];
		uint32_t m_activeReadbackObjects[MAX_READBACK_REQUESTS];
		std::atomic<uint32_t> m_activeReadbackObjectCount;
		DeferredFreeArrayIndexAllocator m_freeReadbackObjects;

	};

}