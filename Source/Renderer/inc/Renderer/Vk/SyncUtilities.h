#pragma once

#include <Renderer/Vk/CommonVk.h>
#include <Renderer/Shared/GfxTypes.h>
#include <vector>

namespace YAPT
{
	class RingSyncUtility
	{
	public:
		RingSyncUtility();
		~RingSyncUtility();

		void initialize(VkDevice device, size_t numberOfEntries, bool createFences, bool createSemaphores);
		void deinitialize();

		void nextFrame();
		void waitForNextFrame();

		void markThisFrameSyncDataIssued();

		VkFence getFenceForThisFrame() const;
		VkSemaphore getSemaphoreForThisFrame() const;

		VkFence getFenceForFrameIndex(size_t index) const;
		VkSemaphore getSemaphoreForFrameIndex(size_t index) const;

		VkFence getFenceForFrameNumber(size_t frameNumber) const;
		VkSemaphore getSemaphoreForFrameNumber(size_t frameNumber) const;

		size_t getFrameCount() const { return m_tickCount; }
		size_t getFrameIndex() const { return m_entryDataIndex; }
		size_t getFrameIndexForFrameNumber(size_t frameNumber) const { return frameNumber % getFramesInFlight(); }

		size_t getFramesInFlight() const { return m_syncData.size(); }

	private:

		struct EntryData
		{
			VkFence fence;
			VkSemaphore semaphore;
			bool fenceWaitPending;
		};
		VkDevice m_device;
		std::vector<EntryData> m_syncData;
		size_t m_tickCount;
		size_t m_entryDataIndex;
	};
}