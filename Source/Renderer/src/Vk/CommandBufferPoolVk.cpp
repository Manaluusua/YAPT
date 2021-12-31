#include <Renderer/Vk/CommandBufferPoolVk.h>
#include <assert.h>

namespace YAPT
{
	CommandBufferPoolVk::CommandBufferPoolVk(VkDevice device)
		:m_device(device)
	{

	}
	CommandBufferPoolVk::~CommandBufferPoolVk()
	{

	}

	void CommandBufferPoolVk::initialize(size_t numberOfPools, size_t numberOfBuffersPerPool, uint32_t queueFamilyIndex, bool isSecondary)
	{
		deinitialize();
		m_secondary = isSecondary;
		VkCommandPoolCreateInfo info{};
		info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		info.queueFamilyIndex = queueFamilyIndex;
		info.flags = 0;


		m_pools.resize(numberOfPools);
		for (size_t i = 0; i < m_pools.size(); ++i)
		{
			checkVkResult(vkCreateCommandPool(m_device, &info, VK_ALLOC_CB, &m_pools[i].pool));

			VkCommandBufferAllocateInfo bufferAlloc{};
			bufferAlloc.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
			bufferAlloc.commandPool = m_pools[i].pool;
			bufferAlloc.level = m_secondary ? VK_COMMAND_BUFFER_LEVEL_SECONDARY :  VK_COMMAND_BUFFER_LEVEL_PRIMARY;
			bufferAlloc.commandBufferCount = (uint32_t)numberOfBuffersPerPool;

			m_pools[i].buffers.resize(numberOfBuffersPerPool);
			checkVkResult(vkAllocateCommandBuffers(m_device, &bufferAlloc, m_pools[i].buffers.data()));

		}
	}
	void CommandBufferPoolVk::deinitialize()
	{
		for (size_t i = 0; i < m_pools.size(); ++i)
		{
			vkDestroyCommandPool(m_device, m_pools[i].pool, VK_ALLOC_CB);
		}

		m_pools.clear();
	}

	void CommandBufferPoolVk::clearPool(size_t index, bool resetResources)
	{
		assert(index < m_pools.size());
		vkResetCommandPool(m_device, m_pools[index].pool, resetResources ? VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT : 0);
	}
	VkCommandBuffer CommandBufferPoolVk::getCommandBuffer(size_t poolIndex, size_t bufferIndex)
	{
		assert(poolIndex < m_pools.size());
		assert(bufferIndex < m_pools[poolIndex].buffers.size());

		return m_pools[poolIndex].buffers[bufferIndex];

	}

	VkCommandBuffer CommandBufferPoolVk::beginCommandBufferRecording(size_t poolIndex, size_t bufferIndex)
	{
		VkCommandBufferBeginInfo cmdBufferBegin{};
		cmdBufferBegin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		cmdBufferBegin.flags = m_secondary ? 0 : VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		VkCommandBuffer buff = getCommandBuffer(poolIndex, bufferIndex);
		checkForVkError(vkBeginCommandBuffer(buff, &cmdBufferBegin));
		return buff;
	}
	void CommandBufferPoolVk::endCommandBufferRecording(size_t poolIndex, size_t bufferIndex)
	{
		endCommandBufferRecording(getCommandBuffer(poolIndex, bufferIndex));
	}

	void CommandBufferPoolVk::endCommandBufferRecording(VkCommandBuffer buff)
	{
		checkForVkError(vkEndCommandBuffer(buff));
	}

}