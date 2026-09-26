#include <Gfx/Dx12/SubmissionThreadDx12.h>

namespace YAPT
{

	SubmissionThreadDx12::SubmissionThreadDx12()
		:m_submitThreadActive(false)
	{
		
	}
	SubmissionThreadDx12::~SubmissionThreadDx12()
		
	{

	}

	void SubmissionThreadDx12::initialize(const Configuration& config)
	{
		for (size_t i = 0; i < COMMANDQUEUETYPE_COUNT; ++i)
		{
			m_queues[i].resize(config.numberOfQueues[i]);
			for (size_t k = 0; k < config.numberOfQueues[i]; ++k)
			{
				m_queues[i][k] = config.queues[i][k];
				
			}
		}

		m_processed = m_submitted = 1;

		startSubmitThread();
	}
	void SubmissionThreadDx12::deinitialize()
	{
		stopSubmitThread();

	}

	void SubmissionThreadDx12::startSubmitThread()
	{
		assert(!m_submitThreadActive);
		m_submitThreadActive = true;
		m_submissionThread = std::thread(SubmissionThreadDx12::threadEntry, this);
		
	}
	void SubmissionThreadDx12::stopSubmitThread()
	{
		m_submitThreadActive.store(false);
		m_submissionThread.join();
	}

	SubmissionThreadDx12::SubmissionId SubmissionThreadDx12::wait(CommandQueueType commandQueueType, size_t commandQueueIndex, FenceValueDx12* fences, size_t numberOfFences)
	{
		Task* t = m_taskQueue.getNextForWriting();
		t->type = TaskType::WAIT;
		t->commandQueueType = commandQueueType;
		t->commandQueueIndex = commandQueueIndex;
		t->fences.assign(fences, fences + numberOfFences);

		m_taskQueue.commit();

		return SubmissionThreadDx12::SubmissionId{ m_submitted.fetch_add(1, std::memory_order_relaxed) };
	}
	SubmissionThreadDx12::SubmissionId SubmissionThreadDx12::signal(CommandQueueType commandQueueType, size_t commandQueueIndex, FenceValueDx12* fences, size_t numberOfFences)
	{
		Task* t = m_taskQueue.getNextForWriting();
		t->type = TaskType::SIGNAL;
		t->commandQueueType = commandQueueType;
		t->commandQueueIndex = commandQueueIndex;
		t->fences.assign(fences, fences + numberOfFences);

		m_taskQueue.commit();

		return SubmissionThreadDx12::SubmissionId{ m_submitted.fetch_add(1, std::memory_order_relaxed) };
	}
	SubmissionThreadDx12::SubmissionId SubmissionThreadDx12::submit(CommandQueueType commandQueueType, size_t commandQueueIndex, ID3D12CommandList** commandLists, size_t numberOfCommandLists)
	{
		Task* t = m_taskQueue.getNextForWriting();
		t->type = TaskType::SUBMIT;
		t->commandQueueType = commandQueueType;
		t->commandQueueIndex = commandQueueIndex;
		t->commandLists.assign(commandLists, commandLists + numberOfCommandLists);

		m_taskQueue.commit();

		return SubmissionThreadDx12::SubmissionId{ m_submitted.fetch_add(1, std::memory_order_relaxed) };

	}

	void SubmissionThreadDx12::issueCallback(SubmissionThreadDx12Callback callback, void* usrData)
	{
		Task* t = m_taskQueue.getNextForWriting();
		t->type = TaskType::CUSTOM_CALLBACK;
		t->callback = callback;
		t->usrData = usrData;

		m_taskQueue.commit();
	}

	bool SubmissionThreadDx12::isPending(SubmissionId id)
	{
		return id.val >= m_processed.load();
	}

	void SubmissionThreadDx12::submitLoop()
	{
		while (m_submitThreadActive.load())
		{
			//poptask

			Task* t = m_taskQueue.peekNextForReading();
			if (t)
			{
				switch (t->type)
				{
				case TaskType::WAIT:
				{
					ID3D12CommandQueue* queue = m_queues[t->commandQueueType][t->commandQueueIndex];
					for (size_t i = 0; i < t->fences.size(); ++i)
					{
						queue->Wait(t->fences[i].fence, t->fences[i].value);
					}
				}
				break;
				case TaskType::SIGNAL:
				{
					ID3D12CommandQueue* queue = m_queues[t->commandQueueType][t->commandQueueIndex];
					for (size_t i = 0; i < t->fences.size(); ++i)
					{
						queue->Signal(t->fences[i].fence, t->fences[i].value);
					}
				}
				break;
				case TaskType::SUBMIT:
				{
					ID3D12CommandQueue* queue = m_queues[t->commandQueueType][t->commandQueueIndex];
					queue->ExecuteCommandLists((UINT)t->commandLists.size(), t->commandLists.data());
				}
				break;

				case TaskType::CUSTOM_CALLBACK:
					t->callback(t->usrData);
					break;
				}
			
				bool success = m_taskQueue.popNext();
				if (t->type != TaskType::CUSTOM_CALLBACK)
				{
					m_processed.fetch_add(1);
				}
				assert(success);
			}
			else
			{
				std::this_thread::sleep_for(std::chrono::microseconds(1));
			}
			

		}
	}

	void SubmissionThreadDx12::threadEntry(SubmissionThreadDx12* submission)
	{
		submission->submitLoop();
	}
}