#include <Common/ThreadPool.h>

namespace YAPT
{


//ThreadPool ---------------------------------------------------------------------------------------------------------------------------
	ThreadPool::ThreadPool(size_t taskQueueSize)
	{
		m_tasks.resize(taskQueueSize);
		
	}

	ThreadPool::~ThreadPool()
	{
		deinit();
	}

	void ThreadPool::addTask(TaskFunc func, void* usrData)
	{
		size_t taskID = m_head.load();
		size_t maxHead = m_completed.load() + m_tasks.size() - 1;

		while (taskID > maxHead)
		{
			// sleep
			maxHead = m_completed.load() + m_tasks.size() - 1;
		}
		size_t index = taskID % m_tasks.size();

		m_tasks[index] = { func, usrData };

		m_head.fetch_add(1);

		m_workAvailableCondition.notify_one();
	}

	void ThreadPool::init(size_t numberOfWorkers)
	{
		deinit();
		m_shutDown = false;
		m_head = m_tail = m_completed = 0;

		m_workers.reserve(numberOfWorkers);
		for (size_t i = 0; i < numberOfWorkers; ++i)
		{
			m_workers.emplace_back(ThreadPool::workerLoopEntry, this);
		}


	}
	void ThreadPool::deinit()
	{
		m_shutDown = true;
		m_workAvailableCondition.notify_all();
		for (size_t i = 0; i < m_workers.size(); ++i)
		{
			m_workers[i].join();
		}

		m_workers.clear();
	}


	bool ThreadPool::haveAllTasksCompleted() const
	{
		return m_head.load() == m_completed.load();
	}

	void ThreadPool::waitForAllTasksCompleted()
	{
		//TODO: add conditional variable to wake up once tasks done rather than this
		while (!haveAllTasksCompleted())
		{
			std::this_thread::sleep_for(std::chrono::microseconds(1));
		}
	}

	void ThreadPool::workerLoop()
	{
		while (true)
		{
			if(!tasksAvailable())
			{
				std::unique_lock<std::mutex> lock(m_workAvailableMutex);
				m_workAvailableCondition.wait(lock, [this] {return m_shutDown || tasksAvailable(); });
			}

			if (m_shutDown)
			{
				return;
			}

			size_t taskID = popNextTask();

			if (taskID == size_t(-1)) continue;

			size_t index = taskID % m_tasks.size();

			Task task = m_tasks[index];
			task.taskFunc(task.usrData);

			m_completed.fetch_add(1);
		}
	}

	bool ThreadPool::tasksAvailable() const
	{
		return m_tail.load() < m_head.load();
	}

	size_t ThreadPool::popNextTask()
	{
		size_t current = m_tail.load();
		if (current < m_head.load())
		{
			if (!m_tail.compare_exchange_weak(current, current + 1))
			{
				current = size_t(-1);
			}
		}
		else
		{
			current = size_t(-1);
		}

		return current;
	}

	void ThreadPool::workerLoopEntry(ThreadPool* threadPool)
	{
		threadPool->workerLoop();
	}
}