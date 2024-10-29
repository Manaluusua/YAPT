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
			triangles.vertexData.deviceAddress = m_resMngr.GetDeviceAddress(geoDef.vertexBuffer->buffer);
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


	void BottomLevelAccelerationStructure::allocate()
	{

	}
	void BottomLevelAccelerationStructure::build()
	{

	}

}