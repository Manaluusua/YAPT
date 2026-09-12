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
		bool issueDownloads(ReadbackDefinitions& def, VkSemaphore* semaphoresToWait, size_t semaphoresToWaitCount, VkSemaphore& signaledSemaphore, FenceHandle fenceToSignal);
	};

}
