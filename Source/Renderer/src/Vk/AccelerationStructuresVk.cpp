#include <Renderer/Vk/AccelerationStructuresVk.h>
#include <Common/CommonUtilities.h>
#include <Renderer/Vk/ResourceManagerVk.h>
#include <Renderer/Vk/YaptToVkConversions.h>
#include <Renderer/Vk/ResourceHandlesVk.h>
#include <vector>

namespace YAPT
{
	void copyTransform(const float src[12], VkTransformMatrixKHR& target)
	{
		memcpy(target.matrix, src, 12 * 4);
	}

	

	//Bottom level acceleration structure
	BottomLevelAccelerationStructure::BottomLevelAccelerationStructure(ResourceManagerVk& resMngr, const BottomLevelAccelerationStructureDefinition& def)
		:m_resMngr(resMngr),
		m_accStruct(VK_NULL_HANDLE)
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
				triangles.transformData.hostAddress = NULL;
				triangles.transformData.deviceAddress = m_resMngr.GetDeviceAddress(geoDef.worldMatrixBuffer->buffer) + geoDef.worldMatrixBufferOffsetInBytes;
			}
			else
			{
				triangles.transformData.hostAddress = NULL;
				triangles.transformData.deviceAddress = NULL;
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
		assert(m_accStruct == VK_NULL_HANDLE);

		m_buildMode = buildMode;
		m_flags = flags;

		VkAccelerationStructureBuildGeometryInfoKHR buildInfo{};
		buildInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
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
			
		m_sizesInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
		m_sizesInfo.pNext = NULL;

		m_resMngr.getVkExtFuncs().vkGetAccelerationStructureBuildSizesKHR(m_resMngr.getDevice(), VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &buildInfo, primitiveCounts.data(), &m_sizesInfo);

		m_accStructBuffer.alloc(m_resMngr, m_sizesInfo.accelerationStructureSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		{
			//create the acceleration structure object
			VkAccelerationStructureCreateInfoKHR createInfo = {};
			createInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
			createInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
			createInfo.size = m_sizesInfo.accelerationStructureSize;
			createInfo.createFlags = 0;
			createInfo.offset = 0;
			createInfo.buffer = m_accStructBuffer.buffer;
			createInfo.deviceAddress = 0;

			VkResult res = m_resMngr.getVkExtFuncs().vkCreateAccelerationStructureKHR(m_resMngr.getDevice(), &createInfo, VK_ALLOC_CB, &m_accStruct);
			checkVkResult(res);
		}
	}

	void BottomLevelAccelerationStructure::deallocate()
	{
		assert(m_accStruct != VK_NULL_HANDLE);
		m_accStructBuffer.dealloc(m_resMngr);
		m_resMngr.deferredDestroyVkResource(m_accStruct);
		m_accStruct = VK_NULL_HANDLE;
	}

	void BottomLevelAccelerationStructure::fillBuildInfo(VkAccelerationStructureBuildGeometryInfoKHR* infoOut, const VkAccelerationStructureBuildRangeInfoKHR*& buildRanges)
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
		buildRanges = m_buildRanges.data();
	}




	//Top level acceleration structure

	TopLevelAccelerationStructure::TopLevelAccelerationStructure(ResourceManagerVk& resMngr, const TopLevelAccelerationStructureDefinition& def)
		:m_resMngr(resMngr),
		m_accStruct(VK_NULL_HANDLE)
	{
		m_instances.resize(def.instanceDefinitionCount);


		for (size_t i = 0; i < def.instanceDefinitionCount; ++i)
		{
			const AccelerationStructureInstanceDefinition& instancesDef = def.instanceDefinitions[i];
			VkAccelerationStructureInstanceKHR& asInst = m_instances[i];
			asInst.mask = instancesDef.instanceMask;
			asInst.instanceShaderBindingTableRecordOffset = instancesDef.hitGroupShaderTableOffset;
			asInst.accelerationStructureReference = instancesDef.blas->getAccelerationStructureDeviceAddress();
			asInst.instanceCustomIndex = instancesDef.instanceID;
			copyTransform(instancesDef.instanceToWorld, asInst.transform);
		}
		
	}


	void TopLevelAccelerationStructure::allocate(VkBuildAccelerationStructureModeKHR buildMode, VkBuildAccelerationStructureFlagsKHR flags)
	{

		assert(m_accStruct == VK_NULL_HANDLE);

		m_buildMode = buildMode;
		m_flags = flags;

		//allocate & upload the instances struct data
		{
			uint32_t bufferSize = (uint32_t)m_instances.size() * sizeof(VkAccelerationStructureInstanceKHR);
			m_instancesBuildDefinitionsBuffer.alloc(m_resMngr, bufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

			void* mappedPtr;
			VkResult res = vkMapMemory(m_resMngr.getDevice(), m_instancesBuildDefinitionsBuffer.deviceMemory, 0, bufferSize, 0, &mappedPtr);
			checkVkResult(res);
			memcpy(mappedPtr, m_instances.data(), bufferSize);
			vkUnmapMemory(m_resMngr.getDevice(), m_instancesBuildDefinitionsBuffer.deviceMemory);
		}

		m_instanceStructsDef = {};
		m_instanceStructsDef.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
		m_instanceStructsDef.data.deviceAddress = m_instancesBuildDefinitionsBuffer.deviceAddress;
		m_instanceStructsDef.arrayOfPointers = VK_FALSE;

		m_accStructGeometry = {};
		m_accStructGeometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
		m_accStructGeometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
		m_accStructGeometry.geometry.instances = m_instanceStructsDef;

		VkAccelerationStructureBuildGeometryInfoKHR buildInfo{};
		buildInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
		buildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
		buildInfo.mode = buildMode;
		buildInfo.flags = flags;
		buildInfo.geometryCount = 1;
		buildInfo.pGeometries = &m_accStructGeometry;

		m_sizesInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
		m_sizesInfo.pNext = NULL;

		uint32_t instanceCount = (uint32_t)m_instances.size();

		m_buildRange.firstVertex = 0;
		m_buildRange.primitiveCount = instanceCount;
		m_buildRange.primitiveOffset = 0;
		m_buildRange.transformOffset = 0;

		m_resMngr.getVkExtFuncs().vkGetAccelerationStructureBuildSizesKHR(m_resMngr.getDevice(), VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &buildInfo, &instanceCount, &m_sizesInfo);

		m_accStructBuffer.alloc(m_resMngr, m_sizesInfo.accelerationStructureSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		{
			//create the acceleration structure object
			VkAccelerationStructureCreateInfoKHR createInfo = {};
			createInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
			createInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
			createInfo.size = m_sizesInfo.accelerationStructureSize;
			createInfo.createFlags = 0;
			createInfo.offset = 0;
			createInfo.buffer = m_accStructBuffer.buffer;
			createInfo.deviceAddress = 0;

			VkResult res = m_resMngr.getVkExtFuncs().vkCreateAccelerationStructureKHR(m_resMngr.getDevice(), &createInfo, VK_ALLOC_CB, &m_accStruct);
			checkVkResult(res);
		}
	}

	void TopLevelAccelerationStructure::deallocate()
	{
		assert(m_accStruct != VK_NULL_HANDLE);

		m_accStructBuffer.dealloc(m_resMngr);
		m_instancesBuildDefinitionsBuffer.dealloc(m_resMngr);

		m_resMngr.deferredDestroyVkResource(m_accStruct);
		m_accStruct = VK_NULL_HANDLE;
	}

	void TopLevelAccelerationStructure::fillBuildInfo(VkAccelerationStructureBuildGeometryInfoKHR* infoOut, const VkAccelerationStructureBuildRangeInfoKHR*& buildRanges)
	{


		VkAccelerationStructureBuildGeometryInfoKHR buildInfo{};
		buildInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
		buildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
		buildInfo.mode = m_buildMode;
		buildInfo.flags = m_flags;
		buildInfo.geometryCount = 1;
		buildInfo.pGeometries = &m_accStructGeometry;
		buildInfo.dstAccelerationStructure = m_accStruct;

		*infoOut = buildInfo;
		buildRanges = &m_buildRange;
	}
}