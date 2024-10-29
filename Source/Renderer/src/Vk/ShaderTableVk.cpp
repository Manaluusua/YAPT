#include <Renderer/Vk/ShaderTableVk.h>
#include <Common/CommonUtilities.h>
#include <renderer/Vk/RendererVk.h>
#include <Renderer/Vk/RaytracePipelineStateVk.h>
#include <Renderer/Vk/ResourceManagerVk.h>
#include <Renderer/Vk/ResourceHandlesVk.h>

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
		m_pso(*pso),
		m_buffer(VK_NULL_HANDLE),
		m_memory(VK_NULL_HANDLE)
	{
		VkDevice device = m_renderer.getDevice();
		const auto& props = m_renderer.getPhysicalDeviceInfo().raytracePipelineProperties;
		m_shaderGroupHandleSize = props.shaderGroupHandleSize;
		//calculate entry sizes
		{
			VkDeviceSize rayGenConstantsSize = m_pso.getRayGenConstantsSizeInBytes();
			VkDeviceSize missConstantsSize = m_pso.getMissConstantsSizeInBytes();
			VkDeviceSize hitConstantsSize = m_pso.getHitGroupConstantsSizeInBytes();

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
		VkDeviceSize sizeSum = m_rayGenRegion.size + m_missRegion.size + m_hitRegion.size;
		//allocate buffer and fill buffer addresses to regions
		{
			createBuffer(sizeSum);
			VkBufferDeviceAddressInfo info{ VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO, nullptr, m_buffer->buffer };
			VkDeviceAddress bufferAddress = vkGetBufferDeviceAddress(device, &info);
			m_rayGenRegion.deviceAddress = bufferAddress;
			m_missRegion.deviceAddress = bufferAddress + m_rayGenRegion.size;
			m_hitRegion.deviceAddress = m_missRegion.deviceAddress + m_missRegion.size;
		}
		m_shadowBuffer.resize(sizeSum);

		clearShadowBufferRange();
	}

	void ShaderTableVk::createBuffer(VkDeviceSize bufferSize)
	{
		ResourceManagerVk& mngr = *m_renderer.getResourceManager();

		VkMemoryRequirements memoryReq;

		//create buffer
		{
			BufferDesc buffDesc(RESOURCE_USAGE_SHADERTABLE_BUFFER | RESOURCE_USAGE_COPY_DESTINATION, bufferSize);
			ResourceStateDescription stateDesc = ResourceStateDescription::default();

			m_buffer = m_renderer.getResourceManager()->createBuffer(buffDesc, stateDesc, m_renderer.getGraphicsQueue().queueFamilyIndex, "ShaderTable");
			vkGetBufferMemoryRequirements(mngr.getDevice(), m_buffer->buffer, &memoryReq);
		}

		//memory
		{
			ResourceManagerVk::AllocatedMemoryInfo memInfo;
			bool success = mngr.allocateDeviceMemory(memoryReq.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, memoryReq.size, false, memInfo);
			m_memory = memInfo.memory;
			assert(success);
		}
		
		{

			VkResult res = vkBindBufferMemory(mngr.getDevice(), m_buffer->buffer, m_memory, 0);
			checkVkResult(res);
		}

	}
	void ShaderTableVk::releaseBuffer()
	{
		m_renderer.getResourceManager()->destroyBuffer(m_buffer);
		m_renderer.getResourceManager()->deferredDestroyVkResource(m_memory);

		m_buffer = nullptr;
		m_memory = VK_NULL_HANDLE;
	}

	void ShaderTableVk::writeShaderTableEntries(const ShaderTableEntry* rayGenBindings, size_t numberOfRayGenBindings,
		const ShaderTableEntry* missBindings, size_t numberOfMissBindings,
		const ShaderTableEntry* hitGroupBindings, size_t numberOfHitGroupBindings)
	{
		for (size_t i = 0; i < numberOfRayGenBindings; ++i)
		{
			writeShaderTableEntry(getRayGenSectionBufferOffset(), m_rayGenRegion.stride,  m_pso.getRayGenShaderRecordHandle(rayGenBindings[i].shaderIndexInPso), rayGenBindings[i]);
		}

		for (size_t i = 0; i < numberOfMissBindings; ++i)
		{
			writeShaderTableEntry(getMissSectionBufferOffset(), m_missRegion.stride, m_pso.getMissShaderRecordHandle(missBindings[i].shaderIndexInPso), missBindings[i]);
		}

		for (size_t i = 0; i < numberOfHitGroupBindings; ++i)
		{
			writeShaderTableEntry(getHitSectionBufferOffset(), m_hitRegion.stride, m_pso.getHitShaderRecordHandle(hitGroupBindings[i].shaderIndexInPso), hitGroupBindings[i]);
		}

	}

	void ShaderTableVk::writeShaderTableEntry(size_t offsetToBufferStart, size_t alignedEntrySize, uint8_t* record, const ShaderTableEntry& entry)
	{
		assert(alignedEntrySize >= (m_shaderGroupHandleSize + entry.extraDataInBytes));
		size_t bufferOffset = offsetToBufferStart + entry.shaderTableIndex * alignedEntrySize;

		memcpy(m_shadowBuffer.data() + bufferOffset, record, m_shaderGroupHandleSize);
		memcpy(m_shadowBuffer.data() + bufferOffset + m_shaderGroupHandleSize, entry.shaderTableExtraData, entry.extraDataInBytes);

		m_shadowBufferUploadRangeMin = std::min(m_shadowBufferUploadRangeMin, bufferOffset);
		m_shadowBufferUploadRangeMax = std::max(m_shadowBufferUploadRangeMax, bufferOffset + alignedEntrySize);
	}

	void ShaderTableVk::uploadShadowBufferData()
	{
		m_renderer.getResourceManager()->upload(m_buffer, m_shadowBufferUploadRangeMin, m_shadowBufferUploadRangeMax - m_shadowBufferUploadRangeMin, m_shadowBuffer.data(), GpuUploadStage::DURING_RENDER);
	}
	void ShaderTableVk::clearShadowBufferRange()
	{
		m_shadowBufferUploadRangeMin = m_shadowBuffer.size();
		m_shadowBufferUploadRangeMax = 0;
	}
}