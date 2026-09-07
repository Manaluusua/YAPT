#pragma once

#include <shared_mutex>
#include <atomic>
#include <xxhash.h>
#include <Gfx/GfxTypes.h>
#include <common/ArrayIndexAllocator.h>

namespace YAPT
{
	template<typename Primitive, typename Implementation, uint32_t MaxPrimitiveCount>
	class SyncPrimitiveManager
	{
	public:
		template <typename... Args>
		SyncPrimitiveManager(Args&&.. args)
			:m_impl(std::forward<Args>(args)..),
			m_indexAllocators((MaxPrimitiveCount + 31) / 32)
		{
			m_creationState.fill(CreationStateNotInitialized);
			m_pendingFreeCount = 0;
		}
		virtual ~SyncPrimitiveManager()
		{

		}

		FenceHandle acquireFence()
		{
			uint32_t index = m_indexAllocators.allocate();
			if (index == INVALID_INDEX) return InvalidFenceHandle;

			assert(m_stateFlags[index] != CreationStateInUse);

			if (m_stateFlags[index] == CreationStateNotInitialized)
			{
				m_primitives[index] = m_impl.createFence();
				m_stateFlags[index] = CreationStateInUse;
			}
			else
			{
				m_impl.resetFence(m_primitives[index]);
			}

			return m_primitives[index];

		}
		void freeFence(FenceHandle ind)
		{
			if (ind == InvalidFenceHandle) return;

			if (getFenceState(ind) == FenceState::Pending)
			{
				uint32_t pendingListIndex = m_pendingFreeCount.fetch_add(1);
				m_pendingFreePrimitives[pendingListIndex] = ind;
			}
			else
			{
				m_indexAllocators.free(ind);
			}

			
		}
		FenceState getFenceState(FenceHandle ind)
		{
			if (ind == InvalidFenceHandle) return FenceState::Unused;
			if (m_creationState[ind] == CreationStateNotInitialized) return FenceState::Unused;
			return m_impl.getFenceState(m_primitives[ind]);
		}

		void syncPendingFree()
		{
			uint32_t freeCount = m_pendingFreeCount;
			for (int i = 0; i < freeCount; ++i)
			{
				if (getState(m_pendingFreePrimitives{ i }) != FenceState::Pending)
				{
					std::swap(m_pendingFreePrimitives[i], m_pendingFreePrimitives[freeCount - 1]);
					--freeCount;
					--i;
				}
			}
			m_pendingFreeCount = freeCount;
		}

	protected:


	private:

		constexpr uint8_t CreationStateNotInitialized = 0;
		constexpr uint8_t CreationStateInUse = 1;
		constexpr uint8_t CreationStateFreed = 2;
		Implementation m_impl;
		std::array<Primitive, MaxPrimitiveCount> m_primitives;
		std::array<uint32_t, MaxPrimitiveCount> m_pendingFreePrimitives;
		std::array<uint8_t, MaxPrimitiveCount> m_creationState;
		ArrayIndexAllocator m_indexAllocators;
		std::atomic<uint32_t> m_pendingFreeCount;
	};

}