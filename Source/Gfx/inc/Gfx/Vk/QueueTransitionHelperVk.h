#pragma once


#include <Gfx/Vk/CommonVk.h>
#include <Gfx/Vk/SyncUtilities.h>
#include <Gfx/Vk/CommandBufferPoolVk.h>
#include <unordered_map>
namespace YAPT
{
	class SubmissionThreadVk;

	class QueueTransitionHelperVk
	{
	public:
		QueueTransitionHelperVk();
		~QueueTransitionHelperVk();

		void addFromBarriers(VkBufferMemoryBarrier* barriers, size_t numberOfBarriers, bool isReleaseBarrier);
		void addFromBarriers(VkImageMemoryBarrier* barriers, size_t numberOfBarriers, bool isReleaseBarrier);

		void clearBarriers();

		bool issueTransitionBarriers(SubmissionThreadVk& submitThread, size_t transitionIndex, VkSemaphore* semaphoresToWait, size_t numberOfSemaphoresToWait, VkSemaphore& lastSignalled);
		void initialize(VkDevice device, size_t pipelineLength, size_t maxNumberOfTransitionsPerFrame);
		void deinitialize();
	private:

		struct QueueFamilyTransitionData
		{
			QueueFamilyTransitionData(VkDevice device, uint32_t numberOfPartitions, size_t maxNumberOfTransitionsPerFrame, uint32_t queueFamilyIndex);
			~QueueFamilyTransitionData();

			CommandBufferPoolVk commandBuffersPool;
			std::vector<RingSyncUtility> syncUtilities;
			std::vector<VkBufferMemoryBarrier> bufferBarriers;
			std::vector<VkImageMemoryBarrier> imageBarriers;
		};

		QueueFamilyTransitionData& getQueueFamilyTransitionData(uint32_t queueFamily);

		std::unordered_map<uint32_t, QueueFamilyTransitionData> m_queueFamilyTransitionData; //TODO: optimize not to use unordered map?
		VkDevice m_device;
		size_t m_pipelineLength;
		VkSemaphore m_lastSignalledSemaphore;
		size_t m_maxNumberOfTransitions;
	};
}