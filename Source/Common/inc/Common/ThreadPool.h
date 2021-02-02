#pragma once

#include <vector>
#include <queue>
#include <atomic>
#include <thread>
#include <mutex>
namespace YAPT
{

	typedef void(*TaskFunc)(void*);
	class ThreadPool
	{
	public:
		ThreadPool(size_t taskQueueSize = 512);
		~ThreadPool();

		void init(size_t numberOfWorkers);
		void deinit();

		//not thread safe!
		void addTask(TaskFunc func, void* usrData);
		
		bool haveAllTasksCompleted() const;

		void waitForAllTasksCompleted();

	private:
		struct Task
		{
			TaskFunc taskFunc;
			void* usrData;
		};

		static void workerLoopEntry(ThreadPool* threadPool);
		void workerLoop();
		
		bool tasksAvailable() const;
		size_t popNextTask();
		
		std::vector<Task> m_tasks;
		std::atomic<size_t> m_head;
		std::atomic<size_t> m_tail;
		std::atomic<size_t> m_completed;

		std::vector<std::thread> m_workers;
		std::condition_variable m_workAvailableCondition;
		std::mutex m_workAvailableMutex;
		bool m_shutDown;
	};
}