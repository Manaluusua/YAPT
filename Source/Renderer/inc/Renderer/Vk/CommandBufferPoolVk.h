#pragma once

#include <Renderer/Vk/CommonVk.h>
#include <vector>

namespace YAPT
{

	class CommandBufferPoolVk
	{

	public:

		CommandBufferPoolVk(VkDevice device);
		~CommandBufferPoolVk();

		void initialize(size_t numberOfPools, size_t numberOfBuffersPerPool, uint32_t queueFamilyIndex, bool isSecondary = false);
		void deinitialize();

		void resetPool(size_t index, bool resetResources = false);
		VkCommandBuffer getCommandBuffer(size_t poolIndex, size_t bufferIndex);
		VkCommandBuffer beginCommandBufferRecording(size_t poolIndex, size_t bufferIndex);
		void endCommandBufferRecording(size_t poolIndex, size_t bufferIndex);
		static void endCommandBufferRecording(VkCommandBuffer buff);

	private:

		struct CommandBufferPoolEntry
		{
			VkCommandPool pool;
			std::vector<VkCommandBuffer> buffers;
		};

		std::vector<CommandBufferPoolEntry> m_pools;

		VkDevice m_device;
		bool m_secondary;
	};

}