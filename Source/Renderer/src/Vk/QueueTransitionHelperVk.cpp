#include <Renderer/Vk/QueueTransitionHelperVk.h>
#include <Renderer/Vk/SubmissionThreadVk.h>

namespace YAPT
{

	QueueTransitionHelperVk::QueueTransitionHelperVk()
		:m_device(VK_NULL_HANDLE),
		m_pipelineLength(0)
	{

	}
	QueueTransitionHelperVk::~QueueTransitionHelperVk()
	{
		deinitialize();
	}

	void QueueTransitionHelperVk::initialize(VkDevice device, size_t pipelineLength, size_t maxNumberOfTransitionsPerFrame)
	{
		m_device = device;
		m_pipelineLength = pipelineLength;
		m_maxNumberOfTransitions = maxNumberOfTransitionsPerFrame;
		
	}

	void QueueTransitionHelperVk::deinitialize()
	{
		for (auto iter = m_queueFamilyTransitionData.begin(); iter != m_queueFamilyTransitionData.end(); ++iter)
		{
			QueueFamilyTransitionData& transitionData = iter->second;
			transitionData.commandBuffersPool.deinitialize();
			for (size_t i = 0; i < m_maxNumberOfTransitions; ++i)
			{
				transitionData.syncUtilities[i].deinitialize();
			}
			
		}
	}

	void QueueTransitionHelperVk::addFromBarriers(VkBufferMemoryBarrier* barriers, size_t numberOfBarriers, bool isReleaseBarrier)
	{
		for (size_t i = 0; i < numberOfBarriers; ++i)
		{
			if (barriers[i].dstQueueFamilyIndex != barriers[i].srcQueueFamilyIndex)
			{
				getQueueFamilyTransitionData(isReleaseBarrier ? barriers[i].srcQueueFamilyIndex : barriers[i].dstQueueFamilyIndex).bufferBarriers.push_back(barriers[i]);
			}
		}

		
	}
	void QueueTransitionHelperVk::addFromBarriers(VkImageMemoryBarrier* barriers, size_t numberOfBarriers, bool isReleaseBarrier)
	{
		for (size_t i = 0; i < numberOfBarriers; ++i)
		{
			if (barriers[i].dstQueueFamilyIndex != barriers[i].srcQueueFamilyIndex)
			{
				getQueueFamilyTransitionData(isReleaseBarrier ? barriers[i].srcQueueFamilyIndex : barriers[i].dstQueueFamilyIndex).imageBarriers.push_back(barriers[i]);
			}
		}
	}

	void QueueTransitionHelperVk::clearBarriers()
	{
		for (auto iter = m_queueFamilyTransitionData.begin(); iter != m_queueFamilyTransitionData.end(); ++iter)
		{
			iter->second.bufferBarriers.clear();
			iter->second.imageBarriers.clear();
		}

	}

	void QueueTransitionHelperVk::resetCommandBuffersForFrame(size_t frameIndex)
	{
		for (auto iter = m_queueFamilyTransitionData.begin(); iter != m_queueFamilyTransitionData.end(); ++iter)
		{
			iter->second.commandBuffersPool.resetPool(frameIndex);
		}
	}

	void QueueTransitionHelperVk::issueTransitionBarriers(SubmissionThreadVk& submitThread, size_t frameIndex, size_t transitionIndex, VkSemaphore* semaphoresToWait, size_t numberOfSemaphoresToWait, VkSemaphore& lastSignalled)
	{
		VkSemaphore latestSignalled = VK_NULL_HANDLE;
		bool isFirstSubmit = true;
		for (auto iter = m_queueFamilyTransitionData.begin(); iter != m_queueFamilyTransitionData.end(); ++iter)
		{
			QueueFamilyTransitionData& transitionData = iter->second;

			if (transitionData.imageBarriers.size() > 0 || transitionData.bufferBarriers.size() > 0)
			{

				VkCommandBuffer cmdBuff = transitionData.commandBuffersPool.beginCommandBufferRecording(frameIndex, transitionIndex);
				vkCmdPipelineBarrier(cmdBuff, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, (uint32_t)transitionData.bufferBarriers.size(), transitionData.bufferBarriers.data(), (uint32_t)transitionData.imageBarriers.size(), transitionData.imageBarriers.data());

				transitionData.commandBuffersPool.endCommandBufferRecording(frameIndex, transitionIndex);

				VkSemaphore signalSemaphore = transitionData.syncUtilities[transitionIndex].getSemaphoreForThisFrame();

				SubmissionThreadVk::Submission submission;

				if (isFirstSubmit)
				{
					submission.semaphoresToWaitCount = numberOfSemaphoresToWait;
					submission.semaphoresToWait = semaphoresToWait;
					isFirstSubmit = false;
				}
				else
				{
					submission.semaphoresToWaitCount = 1;
					submission.semaphoresToWait = &latestSignalled;
				}
				
				submission.semaphoresToSignalCount = 1;
				submission.semaphoresToSignal = &signalSemaphore;

				submission.commandLists = &cmdBuff;
				submission.commandListsCount = 1;

				submission.fenceToSignal = VK_NULL_HANDLE;


				submitThread.submit(iter->first, submission);

				transitionData.syncUtilities[transitionIndex].markThisFrameSyncDataIssued();
				latestSignalled = signalSemaphore;
				transitionData.syncUtilities[transitionIndex].nextFrame();

			}
		}

		lastSignalled = latestSignalled;
	}


	QueueTransitionHelperVk::QueueFamilyTransitionData::QueueFamilyTransitionData(VkDevice device, uint32_t numberOfPartitions, size_t maxNumberOfTransitionsPerFrame, uint32_t queueFamilyIndex)
		:commandBuffersPool(device)
	{
		commandBuffersPool.initialize(numberOfPartitions, maxNumberOfTransitionsPerFrame, queueFamilyIndex);

		syncUtilities.resize(maxNumberOfTransitionsPerFrame);
		for (size_t i = 0; i < maxNumberOfTransitionsPerFrame; ++i)
		{
			syncUtilities[i].initialize(device, numberOfPartitions, false, true);
		}
	}
	QueueTransitionHelperVk::QueueFamilyTransitionData::~QueueFamilyTransitionData()
	{
		commandBuffersPool.deinitialize();

		for (size_t i = 0; i < syncUtilities.size(); ++i)
		{
			syncUtilities[i].deinitialize();
		}

	}

	QueueTransitionHelperVk::QueueFamilyTransitionData& QueueTransitionHelperVk::getQueueFamilyTransitionData(uint32_t queueFamily)
	{
		auto iter = m_queueFamilyTransitionData.find(queueFamily);
		if (iter != m_queueFamilyTransitionData.end())
		{
			return iter->second;
		}
		else
		{
			m_queueFamilyTransitionData.emplace(std::piecewise_construct, std::forward_as_tuple(queueFamily), std::forward_as_tuple(m_device, (uint32_t)m_pipelineLength, m_maxNumberOfTransitions, queueFamily));
			return m_queueFamilyTransitionData.at(queueFamily);

		}
	}
}