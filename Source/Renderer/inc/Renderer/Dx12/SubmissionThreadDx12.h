#pragma once

#include <Renderer/Shared/GfxTypes.h>
#include <Renderer/Dx12/Dx12CommonIncludes.h>
#include <Renderer/Dx12/FenceHelperDx12.h>
#include <thread>
#include <condition_variable>
#include <atomic>
#include <Common/SimpleTaskQueue.h>

#define SUBMIT_QUEUE_MAX_LENGTH 256

namespace YAPT
{
	typedef void(*SubmissionThreadDx12Callback)(void* usrData);
	
	class SubmissionThreadDx12
	{
	public:

		struct SubmissionId
		{
			UINT64 val = 0;
		};

		enum CommandQueueType
		{
			COMMANDQUEUETYPE_GRAPHICS = 0,
			COMMANDQUEUETYPE_COMPUTE,
			COMMANDQUEUETYPE_COPY,
			COMMANDQUEUETYPE_COUNT
		};

		struct Configuration
		{
			ID3D12CommandQueue** queues[COMMANDQUEUETYPE_COUNT];
			size_t numberOfQueues[COMMANDQUEUETYPE_COUNT];
		};

		

		struct Submission
		{
			CommandQueueType commandQueueType;
			size_t commandQueueIndex;
			ID3D12CommandList** commandLists;
			size_t numberOfCommandLists;
		};

		SubmissionThreadDx12();
		~SubmissionThreadDx12();

		void initialize(const Configuration& config);
		void deinitialize();

		SubmissionId wait(CommandQueueType commandQueueType, size_t commandQueueIndex, FenceState* fences, size_t numberOfFences);
		SubmissionId signal(CommandQueueType commandQueueType, size_t commandQueueIndex, FenceState* fences, size_t numberOfFences);
		SubmissionId submit(CommandQueueType commandQueueType, size_t commandQueueIndex, ID3D12CommandList** commandLists, size_t numberOfCommandLists);

		void issueCallback(SubmissionThreadDx12Callback callback, void* usrData);

		bool isPending(SubmissionId id);

	private:
		enum class TaskType
		{
			WAIT,
			SIGNAL,
			SUBMIT,
			CUSTOM_CALLBACK
		};

		//TODO: properly add different types of task data, for now just the superset of everything
		struct Task
		{
			TaskType type;
			CommandQueueType commandQueueType;
			size_t commandQueueIndex;
			std::vector<FenceState> fences;
			std::vector<ID3D12CommandList*> commandLists;
			SubmissionThreadDx12Callback callback;
			void* usrData;
		};

		void startSubmitThread();
		void stopSubmitThread();
		void submitLoop();
		static void threadEntry(SubmissionThreadDx12* submission);

		std::vector<ID3D12CommandQueue*> m_queues[COMMANDQUEUETYPE_COUNT];
		size_t m_numberOfQueues[COMMANDQUEUETYPE_COUNT];

		SimpleTaskQueue<Task, SUBMIT_QUEUE_MAX_LENGTH> m_taskQueue;

		std::thread m_submissionThread;
		std::atomic<bool> m_submitThreadActive;
		std::atomic<UINT64> m_submitted;
		std::atomic<UINT64> m_processed;

	};

}