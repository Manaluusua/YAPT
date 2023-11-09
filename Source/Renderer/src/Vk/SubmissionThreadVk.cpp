#include <Renderer/Vk/SubmissionThreadVk.h>
#include <assert.h>

namespace YAPT
{

	SubmissionThreadVk::SubmissionThreadVk()
		:m_submitThreadActive(false)
	{
		
	}
	SubmissionThreadVk::~SubmissionThreadVk()
		
	{

	}

	void SubmissionThreadVk::initialize(const Configuration& config)
	{
		for (size_t i = 0; i < COMMANDQUEUETYPE_COUNT; ++i)
		{
			m_queues[i].resize(config.numberOfQueues[i]);
			for (size_t k = 0; k < config.numberOfQueues[i]; ++k)
			{
				m_queues[i][k] = config.queues[i][k];
				m_queueFamilyToQueueTypeAndIndex[config.queueFamily[i][k]] = std::make_pair((CommandQueueType)i, (uint32_t)k);
				
			}
		}

		m_processed = m_submitted = 1;

		startSubmitThread();
	}
	void SubmissionThreadVk::deinitialize()
	{
		stopSubmitThread();

	}

	void SubmissionThreadVk::startSubmitThread()
	{
		assert(!m_submitThreadActive);
		m_submitThreadActive = true;
		m_submissionThread = std::thread(SubmissionThreadVk::threadEntry, this);
		
	}
	void SubmissionThreadVk::stopSubmitThread()
	{
		m_submitThreadActive.store(false);
		m_submissionThread.join();
	}

	SubmissionThreadVk::SubmissionId SubmissionThreadVk::submit(uint32_t queueFamilyIndex, const Submission& submission)
	{
		CommandQueueType commandQueueType;
		size_t commandQueueIndex;
		auto iter = m_queueFamilyToQueueTypeAndIndex.find(queueFamilyIndex);
		if (iter != m_queueFamilyToQueueTypeAndIndex.end())
		{
			commandQueueType = iter->second.first;
			commandQueueIndex = iter->second.second;
			return submit(commandQueueType, commandQueueIndex, submission);
		}
		else
		{
			assert(!"Could not map queue family index!");
			return SubmissionThreadVk::SubmissionId{};
		}

		
	}

	SubmissionThreadVk::SubmissionId SubmissionThreadVk::submit(CommandQueueType commandQueueType, size_t commandQueueIndex, const Submission& submission)
	{
		Task* t = m_taskQueue.getNextForWriting();
		t->type = TaskType::SUBMIT;
		t->commandQueueType = commandQueueType;
		t->commandQueueIndex = commandQueueIndex;

		t->commandLists.assign(submission.commandLists, submission.commandLists + submission.commandListsCount);
		t->semaphores.resize(submission.semaphoresToSignalCount + submission.semaphoresToWaitCount);
		t->signalSemaphoreCount = submission.semaphoresToSignalCount;
		t->waitSemaphoreCount = submission.semaphoresToWaitCount;


		for (size_t i = 0; i < submission.semaphoresToWaitCount; ++i)
		{
			t->semaphores[i] = submission.semaphoresToWait[i];
		}

		for (size_t i = 0; i < submission.semaphoresToSignalCount; ++i)
		{
			t->semaphores[submission.semaphoresToWaitCount + i] = submission.semaphoresToSignal[i];
		}

		t->fenceToSignal = submission.fenceToSignal;


		m_taskQueue.commit();

		return SubmissionThreadVk::SubmissionId{ m_submitted.fetch_add(1, std::memory_order_relaxed) };

	}

	SubmissionThreadVk::SubmissionId SubmissionThreadVk::issueCallback(SubmissionThreadVkCallback callback, void* usrData)
	{
		Task* t = m_taskQueue.getNextForWriting();
		t->type = TaskType::CUSTOM_CALLBACK;
		t->callback = callback;
		t->usrData = usrData;

		m_taskQueue.commit();

		return SubmissionThreadVk::SubmissionId{ m_submitted.fetch_add(1, std::memory_order_relaxed) };
	}

	bool SubmissionThreadVk::isPending(SubmissionId id)
	{
		return id.val >= m_processed.load();
	}

	void SubmissionThreadVk::submitLoop()
	{
		std::vector<VkPipelineStageFlags> waitDstFlags;

		while (m_submitThreadActive.load())
		{
			//poptask

			Task* t = m_taskQueue.peekNextForReading();
			if (t)
			{
				switch (t->type)
				{
				case TaskType::SUBMIT:
				{

					VkQueue queue = m_queues[t->commandQueueType][t->commandQueueIndex];
					VkSubmitInfo info{};
					info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
					info.pCommandBuffers = t->commandLists.data();
					info.commandBufferCount = (uint32_t)t->commandLists.size();

					info.pWaitSemaphores = t->semaphores.data();
					info.waitSemaphoreCount = (uint32_t)t->waitSemaphoreCount;
					info.pSignalSemaphores = t->semaphores.data() + t->waitSemaphoreCount;
					info.signalSemaphoreCount = (uint32_t)t->signalSemaphoreCount;
					
					waitDstFlags.resize(t->waitSemaphoreCount, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT);
					info.pWaitDstStageMask = waitDstFlags.data();

					vkQueueSubmit(queue, 1, &info, t->fenceToSignal);

				}
				break;

				case TaskType::CUSTOM_CALLBACK:
					t->callback(t->usrData);
					break;
				}
			
				bool success = m_taskQueue.popNext();
				m_processed.fetch_add(1);
				assert(success);
			}
			else
			{
				std::this_thread::sleep_for(std::chrono::microseconds(1));
			}
			

		}
	}

	void SubmissionThreadVk::threadEntry(SubmissionThreadVk* submission)
	{
		submission->submitLoop();
	}
}