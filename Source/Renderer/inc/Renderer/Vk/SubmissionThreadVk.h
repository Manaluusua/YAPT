#pragma once

#include <Renderer/Shared/GfxTypes.h>
#include <Renderer/Vk/CommonVk.h>

#include <thread>
#include <condition_variable>
#include <atomic>
#include <Common/SimpleTaskQueue.h>
#include <unordered_map>

#define SUBMIT_QUEUE_MAX_LENGTH 256

namespace YAPT
{
	typedef void(*SubmissionThreadVkCallback)(void* usrData);
	
	class SubmissionThreadVk
	{
	public:

		struct SubmissionId
		{
			UINT64 val = 0;
		};

		struct QueueDefinition
		{

		};

		struct Configuration
		{
			VkQueue* queues[COMMANDQUEUETYPE_COUNT];
			size_t numberOfQueues[COMMANDQUEUETYPE_COUNT];
			uint32_t* queueFamily[COMMANDQUEUETYPE_COUNT];
				
		};

		

		struct Submission
		{
			VkCommandBuffer* commandLists = nullptr;
			size_t commandListsCount = 0;
			VkSemaphore* semaphoresToSignal = nullptr;
			size_t semaphoresToSignalCount = 0;
			VkSemaphore* semaphoresToWait = nullptr;
			size_t semaphoresToWaitCount = 0;
			VkFence fenceToSignal = VK_NULL_HANDLE;
		};

		SubmissionThreadVk();
		~SubmissionThreadVk();

		void initialize(const Configuration& config);
		void deinitialize();

		SubmissionId submit(CommandQueueType commandQueueType, size_t commandQueueIndex, const Submission& submission);
		SubmissionId submit(uint32_t queueFamilyIndex, const Submission& submission);

		SubmissionId issueCallback(SubmissionThreadVkCallback callback, void* usrData);

		bool isPending(SubmissionId id);

	private:
		enum class TaskType
		{
			SUBMIT,
			CUSTOM_CALLBACK
		};

		//TODO: properly add different types of task data, for now just the superset of everything
		struct Task
		{
			TaskType type;
			CommandQueueType commandQueueType;
			size_t commandQueueIndex;

			std::vector<VkCommandBuffer> commandLists;
			std::vector<VkSemaphore> semaphores;
			size_t signalSemaphoreCount;
			size_t waitSemaphoreCount;
			VkFence fenceToSignal;
			
			SubmissionThreadVkCallback callback;
			void* usrData;
		};

		void startSubmitThread();
		void stopSubmitThread();
		void submitLoop();
		static void threadEntry(SubmissionThreadVk* submission);

		std::vector<VkQueue> m_queues[COMMANDQUEUETYPE_COUNT];
		size_t m_numberOfQueues[COMMANDQUEUETYPE_COUNT];
		std::unordered_map<uint32_t, std::pair<CommandQueueType, uint32_t>> m_queueFamilyToQueueTypeAndIndex;

		SimpleTaskQueue<Task, SUBMIT_QUEUE_MAX_LENGTH> m_taskQueue;

		std::thread m_submissionThread;
		std::atomic<bool> m_submitThreadActive;
		std::atomic<UINT64> m_submitted;
		std::atomic<UINT64> m_processed;

	};

}