#pragma once

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <vector>
#include <stdint.h>

#include <Common/CommonUtilities.h>

namespace YAPT
{
	typedef void(*JobFunc)(void*);

	struct JobHandle
	{
		uint32_t index = uint32_t(-1);
		uint32_t generation = 0;

		bool isValid() const { return index != uint32_t(-1); }
	};
	class JobSystem
	{
	public:
		YAPT_NOCOPY(JobSystem);

		JobSystem();
		~JobSystem();

		void init(size_t maxJobsPerFrame, size_t allocatedDependenciesPerJob, size_t numWorkerThreads);
		void deinit();

		void reset();
		void waitAllJobs();

		JobHandle submit(JobFunc func, void* userData, const JobHandle* dependencies = nullptr, size_t numDependencies = 0);
		JobHandle combineDependencies(const JobHandle* dependencies, size_t numDependencies)
		{
			return submit(nullptr, nullptr, dependencies, numDependencies);
		}

		void wait(JobHandle job);

	private:
		struct WaiterNode
		{
			WaiterNode* next;
			uint32_t jobIndex;
		};

		struct Job
		{
			JobFunc func = nullptr;
			void* userData = nullptr;
			std::atomic<uint32_t> pendingCount{ 0 };
			std::atomic<WaiterNode*> waiters{ nullptr };
			std::atomic<bool> hasCompleted{ false };
			std::mutex waitMutex;
			std::condition_variable waitCV;
			uint32_t generation = 0;
		};

		bool tryRegisterWaiter(std::atomic<WaiterNode*>& head, WaiterNode* node);
		WaiterNode* completeAndGetWaiters(std::atomic<WaiterNode*>& head);
		static WaiterNode* completedSentinel();

		WaiterNode* allocateWaiterNodes(size_t count);

		void onDependencySatisfied(uint32_t jobIndex);
		void completeJob(uint32_t jobIndex);

		void pushReady(uint32_t jobIndex);
		bool tasksAvailable() const;
		bool popReady(uint32_t& outJobIndex);

		static void workerEntry(JobSystem* self);
		void workerLoop();

		Job* m_jobs = nullptr;
		std::atomic<uint32_t> m_nextJobIndex{ 0 };
		uint32_t m_currentGeneration = 0;

		std::vector<WaiterNode> m_waiterNodePool;
		std::atomic<size_t> m_nextWaiterNode{ 0 };

		std::atomic<uint64_t>* m_readyQueue = nullptr;
		size_t m_readyQueueSize = 0;
		std::atomic<size_t> m_readyHead{ 0 };
		std::atomic<size_t> m_readyTail{ 0 };

		std::vector<std::thread> m_workers;
		std::mutex m_workAvailableMutex;
		std::condition_variable m_workAvailableCV;

		size_t m_maxJobs;
		bool m_shutDown = false;
	};
}
