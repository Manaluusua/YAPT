#pragma once


#include <Gfx/Vk/CopyHelperVk.h>

namespace YAPT
{
	class ResourceManagerVk;
	class SubmissionThreadVk;


	class DownloadHelperVk : public CopyHelperVk
	{

	public:

		DownloadHelperVk(ResourceManagerVk& resourceMngr, SubmissionThreadVk& submitThread, size_t numberOfPartitions, size_t numberOfDownloadsPerFrame);
		~DownloadHelperVk();

		void prepareNextDownloadBatch();

		//the copies read what the frame is rendering, so they are only taken over here and issued once the render work
		//they depend on has actually been submitted
		void queueDownloads(const ReadbackDefinitions& def, FenceHandle fenceToSignal);
		bool hasQueuedDownloads() const { return m_queuedBatches.size() > 0; }
		bool flushDownloadBatches(VkSemaphore* semaphoresToWait, size_t semaphoresToWaitCount, VkSemaphore& signaledSemaphore);

	private:

		//a single queueDownloads() call, issued as one batch of copies with one fence of its own
		struct QueuedDownloadBatch
		{
			size_t textureOffset;
			size_t textureCount;
			size_t bufferOffset;
			size_t bufferCount;
			FenceHandle fenceToSignal;
		};

		void issueDownloads(const QueuedDownloadBatch& batch, VkSemaphore* semaphoresToWait, size_t semaphoresToWaitCount, VkSemaphore& signaledSemaphore);

		//readbacks are only ever queued from the thread executing the render graph, so unlike the uploads these do not
		//need the multi producer pending lists
		std::vector<TextureReadbackDefinition> m_queuedTextureReadbacks;
		std::vector<BufferReadbackDefinition> m_queuedBufferReadbacks;
		std::vector<QueuedDownloadBatch> m_queuedBatches;
	};

}
