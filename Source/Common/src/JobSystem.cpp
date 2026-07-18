#include <Common/JobSystem.h>
#include <Common/Logger.h>
#include <cassert>

namespace YAPT
{

	static uint64_t packReadyEntry(uint32_t generation, uint32_t jobIndex)
	{
		return (uint64_t(generation) << 32) | uint64_t(jobIndex);
	}

	static uint32_t unpackReadyGeneration(uint64_t packed)
	{
		return uint32_t(packed >> 32);
	}

	static uint32_t unpackReadyJobIndex(uint64_t packed)
	{
		return uint32_t(packed & 0xFFFFFFFFu);
	}

	JobSystem::JobSystem()
	{
		
	}

	JobSystem::~JobSystem()
	{
		deinit();
	}

	void JobSystem::init(size_t maxJobsPerFrame, size_t allocatedDependenciesPerJob, size_t numWorkerThreads)
	{
		m_jobs = new Job[maxJobsPerFrame];

		m_readyQueueSize = maxJobsPerFrame;
		m_readyQueue = new std::atomic<uint64_t>[maxJobsPerFrame];
	
		for (size_t i = 0; i < maxJobsPerFrame; ++i)
		{
			m_readyQueue[i].store(packReadyEntry(uint32_t(-1), 0), std::memory_order_relaxed);
		}

		m_waiterNodePool.resize(maxJobsPerFrame * (allocatedDependenciesPerJob + 1));
		m_workers.reserve(numWorkerThreads);
		for (size_t i = 0; i < numWorkerThreads; ++i)
		{
			m_workers.emplace_back(JobSystem::workerEntry, this);
		}

		m_maxJobs = maxJobsPerFrame;

		m_shutDown = false;
	}

	void JobSystem::deinit()
	{
		if (m_shutDown) return;

		m_shutDown = true;
		m_workAvailableCV.notify_all();
		for (auto& worker : m_workers)
		{
			worker.join();
		}

		delete[] m_jobs;
		delete[] m_readyQueue;
	}

	void JobSystem::reset()
	{
		assert(m_readyHead.load() == m_readyTail.load());

		m_nextJobIndex.store(0);
		m_nextWaiterNode.store(0);
		++m_currentGeneration;
	}

	JobHandle JobSystem::submit(JobFunc func, void* userData, const JobHandle* dependencies, size_t numDependencies)
	{
		//if the job submitted is just an empty job without any real dependencies, just early out, nothing to do
		if (func == nullptr)
		{
			bool hasValidDependency = false;
			for (size_t i = 0; i < numDependencies; ++i)
			{
				if (dependencies[i].isValid())
				{
					hasValidDependency = true;
					break;
				}
			}
			if (!hasValidDependency)
			{
				return {};
			}
		}

		uint32_t jobIndex = m_nextJobIndex.fetch_add(1);
		if (jobIndex >= m_maxJobs)
		{
			YAPT_LOG_FATAL_ERROR("JobSystem::submit - exceeded maxJobsPerFrame");
			return JobHandle{};
		}


		Job& job = m_jobs[jobIndex];
		job.func = func;
		job.userData = userData;
		job.generation = m_currentGeneration;
		job.waiters.store(nullptr);
		job.hasCompleted.store(false);

		job.pendingCount.store(uint32_t(numDependencies) + 1);

		for (size_t i = 0; i < numDependencies; ++i)
		{
			const JobHandle& dep = dependencies[i];

			bool alreadySatisfied = true;
			if (dep.isValid() && dep.index < m_maxJobs && m_jobs[dep.index].generation == dep.generation)
			{
				WaiterNode* node = allocateWaiterNodes(1);
				node->jobIndex = jobIndex;
				alreadySatisfied = !tryRegisterWaiter(m_jobs[dep.index].waiters, node);
			}


			if (alreadySatisfied)
			{
				onDependencySatisfied(jobIndex);
			}
		}

		onDependencySatisfied(jobIndex);

		return JobHandle{ jobIndex, m_currentGeneration };
	}

	void JobSystem::wait(JobHandle jobHandle)
	{
		if (!jobHandle.isValid()) return;
		assert(jobHandle.index < m_maxJobs);
		Job& job = m_jobs[jobHandle.index];
		std::unique_lock<std::mutex> lock(job.waitMutex);
		job.waitCV.wait(lock, [&job] { return job.hasCompleted.load(); });
	}

	JobSystem::WaiterNode* JobSystem::completedSentinel()
	{
		return reinterpret_cast<WaiterNode*>(uintptr_t(1));
	}

	bool JobSystem::tryRegisterWaiter(std::atomic<WaiterNode*>& head, WaiterNode* node)
	{
		WaiterNode* current = head.load();
		while (current != completedSentinel())
		{
			node->next = current;
			if (head.compare_exchange_weak(current, node))
			{
				return true;
			}
		}
		return false;
	}

	JobSystem::WaiterNode* JobSystem::completeAndGetWaiters(std::atomic<WaiterNode*>& head)
	{
		return head.exchange(completedSentinel());
	}

	JobSystem::WaiterNode* JobSystem::allocateWaiterNodes(size_t count)
	{
		size_t start = m_nextWaiterNode.fetch_add(count);
		if (start + count > m_waiterNodePool.size())
		{
			YAPT_LOG_FATAL_ERROR("JobSystem - exceeded waiter node pool capacity");
			start = 0;
		}
		return &m_waiterNodePool[start];
	}

	void JobSystem::onDependencySatisfied(uint32_t jobIndex)
	{
		Job& job = m_jobs[jobIndex];
		if (job.pendingCount.fetch_sub(1) == 1)
		{
			pushReady(jobIndex);
		}
	}

	void JobSystem::completeJob(uint32_t jobIndex)
	{
		Job& job = m_jobs[jobIndex];

		WaiterNode* node = completeAndGetWaiters(job.waiters);
		if (node != completedSentinel())
		{
			while (node != nullptr)
			{
				onDependencySatisfied(node->jobIndex);
				node = node->next;
			}
		}

		{
			std::unique_lock<std::mutex> lock(job.waitMutex);
			job.hasCompleted.store(true);
		}
		job.waitCV.notify_all();
	}


	void JobSystem::pushReady(uint32_t jobIndex)
	{
		size_t slot = m_readyHead.fetch_add(1) % m_readyQueueSize;
		m_readyQueue[slot].store(packReadyEntry(m_currentGeneration, jobIndex), std::memory_order_release);

		m_workAvailableCV.notify_one();
	}

	bool JobSystem::tasksAvailable() const
	{
		return m_readyTail.load() < m_readyHead.load();
	}

	bool JobSystem::popReady(uint32_t& outJobIndex)
	{
		size_t current = m_readyTail.load();
		if (current >= m_readyHead.load())
		{
			return false;
		}

		uint64_t packed = m_readyQueue[current % m_readyQueueSize].load(std::memory_order_acquire);
		if (unpackReadyGeneration(packed) != m_currentGeneration)
		{
			return false;
		}

		if (!m_readyTail.compare_exchange_weak(current, current + 1))
		{
			return false;
		}

		outJobIndex = unpackReadyJobIndex(packed);
		return true;
	}

	void JobSystem::workerEntry(JobSystem* self)
	{
		self->workerLoop();
	}

	void JobSystem::workerLoop()
	{
		while (true)
		{
			if (!tasksAvailable())
			{
				std::unique_lock<std::mutex> lock(m_workAvailableMutex);
				m_workAvailableCV.wait(lock, [this] { return m_shutDown || tasksAvailable(); });
			}

			if (m_shutDown)
			{
				return;
			}

			uint32_t jobIndex;
			if (!popReady(jobIndex))
			{
				continue;
			}

			Job& job = m_jobs[jobIndex];
			if (job.func)
			{
				job.func(job.userData);
			}
			completeJob(jobIndex);
		}
	}
}
