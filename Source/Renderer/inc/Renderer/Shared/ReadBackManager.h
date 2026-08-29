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
			:m_state(ReadbackState::Unused),
			m_index(uint32_t(-1))
		{

		}

		virtual ReadbackState getState() final
		{
			return ReadbackState::Pending;
		}

		virtual void allReferencesReleased() final
		{
			setState(ReadbackState::Unused);
		}
	private:
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
		uint32_t m_index;
		std::atomic<ReadbackState> m_state;
	};

	class ReadbackManager 
	{
		friend class CReadbackObject;
	public:
		ReadbackManager(CRenderer& renderer);
		~ReadbackManager();

		void syncToRenderThread();

		ReadbackHandle readback(ReadbackTarget target);
	private:
		static constexpr uint32_t MAX_READBACK_REQUESTS = 256;
		void readbackObjectRelease(CReadbackObject* obj);

		CRenderer& m_renderer;
		CReadbackObject m_readbackObjectPool[MAX_READBACK_REQUESTS];
		DeferredFreeArrayIndexAllocator m_freeReadbackObjects;

	};

}