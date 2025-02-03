#include <Gfx/Vk/SyncUtilities.h>

namespace YAPT
{
	RingSyncUtility::RingSyncUtility()
		:m_device(VK_NULL_HANDLE),
		m_entryDataIndex(0),
		m_tickCount(0)
	{

	}
	RingSyncUtility::~RingSyncUtility()
	{
		deinitialize();
	}

	void RingSyncUtility::initialize(VkDevice device, size_t numberOfEntries, bool createFences, bool createSemaphores)
	{
		m_tickCount = 0;
		m_entryDataIndex = 0;
		m_syncData.resize(numberOfEntries);
		m_device = device;
		for (size_t i = 0; i < m_syncData.size(); ++i)
		{
			if (createFences)
			{
				VkFenceCreateInfo info{};
				info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
				checkVkResult(vkCreateFence(device, &info, VK_ALLOC_CB, &m_syncData[i].fence));
			}
			else
			{
				m_syncData[i].fence = VK_NULL_HANDLE;
			}

			if (createSemaphores)
			{
				VkSemaphoreCreateInfo info{};
				info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
				checkVkResult(vkCreateSemaphore(device, &info, VK_ALLOC_CB, &m_syncData[i].semaphore));
			}
			else
			{
				m_syncData[i].semaphore = VK_NULL_HANDLE;
			}

			m_syncData[i].fenceWaitPending = false;
		}

	}
	void RingSyncUtility::deinitialize()
	{
		for (size_t i = 0; i < m_syncData.size(); ++i)
		{
			if (m_syncData[i].fence != YAPT_NULL_HANDLE)
			{
				vkDestroyFence(m_device, m_syncData[i].fence, VK_ALLOC_CB);
			}

			if (m_syncData[i].semaphore != YAPT_NULL_HANDLE)
			{
				vkDestroySemaphore(m_device, m_syncData[i].semaphore, VK_ALLOC_CB);
			}
		}

		m_syncData.clear();
	}

	void RingSyncUtility::nextFrame()
	{
		waitForNextFrame();
		m_tickCount++;
		m_entryDataIndex = m_tickCount % m_syncData.size();
	}

	void RingSyncUtility::waitForNextFrame()
	{
		size_t nextFrameIndex = (m_tickCount + 1) % m_syncData.size();

		if (m_syncData[nextFrameIndex].fence != YAPT_NULL_HANDLE && m_syncData[nextFrameIndex].fenceWaitPending)
		{
			checkVkResult(vkWaitForFences(m_device, 1, &m_syncData[nextFrameIndex].fence, VK_TRUE, uint64_t(-1)));
			checkVkResult(vkResetFences(m_device, 1, &m_syncData[nextFrameIndex].fence));
		}
		m_syncData[nextFrameIndex].fenceWaitPending = false;
	}

	void RingSyncUtility::markThisFrameSyncDataIssued()
	{
		m_syncData[m_entryDataIndex].fenceWaitPending = true;
	}

	VkFence RingSyncUtility::getFenceForThisFrame() const
	{
		return getFenceForFrameIndex(m_entryDataIndex);
	}
	VkSemaphore RingSyncUtility::getSemaphoreForThisFrame() const
	{
		return getSemaphoreForFrameIndex(m_entryDataIndex);
	}

	VkFence RingSyncUtility::getFenceForFrameNumber(size_t frameNumber) const
	{
		return getFenceForFrameIndex(getFrameIndexForFrameNumber(frameNumber));
	}
	VkSemaphore RingSyncUtility::getSemaphoreForFrameNumber(size_t frameNumber) const
	{
		return getSemaphoreForFrameIndex(getFrameIndexForFrameNumber(frameNumber));
	}

	VkFence RingSyncUtility::getFenceForFrameIndex(size_t index) const
	{
		return m_syncData[index].fence;

	}
	VkSemaphore RingSyncUtility::getSemaphoreForFrameIndex(size_t index) const
	{
		return m_syncData[m_entryDataIndex].semaphore;
	}
}