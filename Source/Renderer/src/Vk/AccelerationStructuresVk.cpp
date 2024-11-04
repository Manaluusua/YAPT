#include <Renderer/Vk/AccelerationStructuresVk.h>
#include <Common/CommonUtilities.h>
#include <Renderer/Vk/ResourceManagerVk.h>
#include <Renderer/Vk/YaptToVkConversions.h>
#include <Renderer/Vk/ResourceHandlesVk.h>
#include <vector>

namespace YAPT
{
	BottomLevelAccelerationStructure::BottomLevelAccelerationStructure(ResourceManagerVk& resMngr, const BottomLevelAccelerationStructureDefinition& def)
		:m_resMngr(resMngr)
	{
		
		m_geometries.resize(def.geometryDefinitionCount);
		m_buildRanges.resize(def.geometryDefinitionCount);

		for (size_t i = 0; i < def.geometryDefinitionCount; ++i)
		{
			const AccelerationStructureGeometryDefinition& geoDef = def.geometryDefinitions[i];
			
			// Describe buffer as array of VertexObj.
			VkAccelerationStructureGeometryTrianglesDataKHR triangles = {};
			triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
			triangles.vertexFormat = yaptFormatToVk(geoDef.vertexFormat);
			triangles.vertexData.deviceAddress = m_resMngr.GetDeviceAddress(geoDef.vertexBuffer->buffer) + geoDef.vertexBufferOffsetInBytes;
			triangles.vertexStride = geoDef.vertexStrideInBytes;
			triangles.indexType = yaptFormatToVkIndex(geoDef.indexFormat);
			triangles.indexData.deviceAddress = m_resMngr.GetDeviceAddress(geoDef.indexBuffer->buffer);

			triangles.maxVertex = (uint32_t)geoDef.vertexCount-1;

			if (geoDef.worldMatrixBuffer)
			{
				triangles.transformData.hostAddress = nullptr;
				triangles.transformData.deviceAddress = m_resMngr.GetDeviceAddress(geoDef.worldMatrixBuffer->buffer);
			}
			else
			{
				triangles.transformData = {};
			}
			

			VkAccelerationStructureGeometryKHR& asGeom = m_geometries[i];
			asGeom.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
			asGeom.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
			asGeom.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
			asGeom.geometry.triangles = triangles;

			VkAccelerationStructureBuildRangeInfoKHR& offset = m_buildRanges[i];
			offset.firstVertex = 0;
			offset.primitiveCount = (uint32_t)geoDef.indexCount / 3;
			offset.primitiveOffset = 0;
			offset.transformOffset = 0;
		}
	}


	void BottomLevelAccelerationStructure::allocate(VkBuildAccelerationStructureModeKHR buildMode, VkBuildAccelerationStructureFlagsKHR flags)
	{
		m_buildMode = buildMode;
		m_flags = flags;

		VkAccelerationStructureBuildGeometryInfoKHR buildInfo{};
		buildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
		buildInfo.mode = buildMode;
		buildInfo.flags = flags;
		buildInfo.geometryCount = (uint32_t)m_geometries.size();
		buildInfo.pGeometries = m_geometries.data();

		std::vector<uint32_t> primitiveCounts(m_buildRanges.size());
		for (uint32_t i = 0; i < m_buildRanges.size(); i++)
		{
			primitiveCounts[i] = m_buildRanges[i].primitiveCount;
		}
			
		m_resMngr.getVkExtFuncs().vkGetAccelerationStructureBuildSizesKHR(m_resMngr.getDevice(), VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &buildInfo, primitiveCounts.data(), &m_sizesInfo);
		

		//memory allocation (TODO: actually pool the memory rather than allocation per structure. But should have a 'generic' memory pool, not just for BLAS/TLAS. 
		{
			VkBufferCreateInfo buffCreateInfo{};
			buffCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
			buffCreateInfo.size = m_sizesInfo.accelerationStructureSize;
			buffCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
			buffCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
			VkResult res = vkCreateBuffer(m_resMngr.getDevice(), &buffCreateInfo, VK_ALLOC_CB, &m_buffer);

			ResourceManagerVk::AllocatedMemoryInfo memInfo;
			bool success = m_resMngr.allocateDeviceMemory(ResourceManagerVk::ALLOW_ALL_MEMORY_TYPES, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_sizesInfo.accelerationStructureSize, true, memInfo);
			m_deviceMemory = memInfo.memory;

			res = vkBindBufferMemory(m_resMngr.getDevice(), m_buffer, m_deviceMemory, 0);

			checkVkResult(res);
			assert(success);
		}
		
		

		{
			//create the acceleration structure object
			VkAccelerationStructureCreateInfoKHR createInfo = {};
			createInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
			createInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
			createInfo.size = m_sizesInfo.accelerationStructureSize;
			createInfo.createFlags = 0;
			createInfo.offset = 0;
			createInfo.buffer = m_buffer;
			createInfo.deviceAddress = 0;

			VkResult res = m_resMngr.getVkExtFuncs().vkCreateAccelerationStructureKHR(m_resMngr.getDevice(), &createInfo, VK_ALLOC_CB, &m_accStruct);
			checkVkResult(res);
		}
		


	}


	void BottomLevelAccelerationStructure::fillBuildInfo(VkAccelerationStructureBuildGeometryInfoKHR* infoOut)
	{
		VkAccelerationStructureBuildGeometryInfoKHR buildInfo{};
		buildInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
		buildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
		buildInfo.mode = m_buildMode;
		buildInfo.flags = m_flags;
		buildInfo.geometryCount = (uint32_t)m_geometries.size();
		buildInfo.pGeometries = m_geometries.data();
		buildInfo.dstAccelerationStructure = m_accStruct;

		*infoOut = buildInfo;
	}

}