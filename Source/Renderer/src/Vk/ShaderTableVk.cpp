#include <Renderer/Vk/ShaderTableVk.h>
#include <Common/CommonUtilities.h>
#include <renderer/Vk/RendererVk.h>
#include <Renderer/Vk/RaytracePipelineStateVk.h>
#include <Renderer/Vk/ResourceManagerVk.h>

namespace YAPT
{
	ShaderTableVk* ShaderTableVk::createShaderTable(RendererVk* renderer, RaytracePipelineStateHandle pso, size_t numberOfRayGenShaders, size_t numberOfMissShaders, size_t numberOfHitGroups)
	{
		
		return new ShaderTableVk(renderer, pso, numberOfRayGenShaders, numberOfMissShaders, numberOfHitGroups);
	}


	ShaderTableVk::~ShaderTableVk()
	{
		releaseBuffer();
	}

	ShaderTableVk::ShaderTableVk(RendererVk* renderer, RaytracePipelineStateHandle pso, size_t numberOfRayGenShaders, size_t numberOfMissShaders, size_t numberOfHitGroups)
		:m_renderer(*renderer),
		m_buffer(VK_NULL_HANDLE),
		m_memory(VK_NULL_HANDLE)
	{
		VkDevice device = m_renderer.getDevice();
		const auto& props = m_renderer.getPhysicalDeviceInfo().raytracePipelineProperties;

		//calculate entry sizes
		{
			VkDeviceSize rayGenConstantsSize = pso->getRayGenConstantsSizeInBytes();
			VkDeviceSize missConstantsSize = pso->getMissConstantsSizeInBytes();
			VkDeviceSize hitConstantsSize = pso->getHitGroupConstantsSizeInBytes();

			VkDeviceSize rayGenEntrySize = align(props.shaderGroupHandleSize + rayGenConstantsSize, props.shaderGroupHandleAlignment);
			VkDeviceSize missEntrySize = align(props.shaderGroupHandleSize + missConstantsSize, props.shaderGroupHandleAlignment);
			VkDeviceSize hitEntrySize = align(props.shaderGroupHandleSize + hitConstantsSize, props.shaderGroupHandleAlignment);

			assert(numberOfRayGenShaders == 1);

			m_rayGenRegion.stride = align(rayGenEntrySize, props.shaderGroupBaseAlignment);
			m_rayGenRegion.size = m_rayGenRegion.stride * numberOfRayGenShaders;

			m_missRegion.stride = missEntrySize;
			m_missRegion.size = align(numberOfMissShaders * missEntrySize, props.shaderGroupBaseAlignment);

			m_hitRegion.stride = hitEntrySize;
			m_hitRegion.size = align(numberOfHitGroups * hitEntrySize, props.shaderGroupBaseAlignment);
		}

		//allocate buffer and fill buffer addresses to regions
		{
			VkDeviceSize sizeSum = m_rayGenRegion.size + m_missRegion.size + m_hitRegion.size;
			createBuffer( sizeSum);
			VkBufferDeviceAddressInfo info{ VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO, nullptr, m_buffer };
			VkDeviceAddress bufferAddress = vkGetBufferDeviceAddress(device, &info);
			m_rayGenRegion.deviceAddress = bufferAddress;
			m_missRegion.deviceAddress = bufferAddress + m_rayGenRegion.size;
			m_hitRegion.deviceAddress = m_missRegion.deviceAddress + m_missRegion.size;
		}

	}

	void ShaderTableVk::createBuffer(VkDeviceSize bufferSize)
	{
		ResourceManagerVk& mngr = *m_renderer.getResourceManager();

		VkMemoryRequirements memoryReq;

		//create buffer
		{
			
			VkBufferCreateInfo buffCreateInfo{};
			buffCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
			buffCreateInfo.size = bufferSize;
			buffCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR;
			buffCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
			VkResult res = vkCreateBuffer(m_renderer.getResourceManager()->getDevice(), &buffCreateInfo, VK_ALLOC_CB, &m_buffer);
			checkVkResult(res);
			vkGetBufferMemoryRequirements(mngr.getDevice(), m_buffer, &memoryReq);
		}

		//memory
		{
			ResourceManagerVk::AllocatedMemoryInfo memInfo;
			bool success = mngr.allocateDeviceMemory(memoryReq.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, memoryReq.size, memInfo);
			m_memory = memInfo.memory;
			assert(success);
		}
		
		{

			VkResult res = vkBindBufferMemory(mngr.getDevice(), m_buffer, m_memory, 0);
			checkVkResult(res);
		}

	}
	void ShaderTableVk::releaseBuffer()
	{
		m_renderer.getResourceManager()->deferredDestroyVkResource(m_buffer);
		m_renderer.getResourceManager()->deferredDestroyVkResource(m_memory);

		m_buffer = VK_NULL_HANDLE;
		m_memory = VK_NULL_HANDLE;
	}
}