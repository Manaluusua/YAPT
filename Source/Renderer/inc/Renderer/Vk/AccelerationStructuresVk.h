#ifndef YAPT_DX12_ACCELERATIONSTRUCTURES_H
#define YAPT_DX12_ACCELERATIONSTRUCTURES_H

#include <Renderer/Shared/GfxTypes.h>
#include <Renderer/Vk/SimpleBufferAllocationUtility.h>

namespace YAPT
{
	class ResourceManagerVk;

	
	class BottomLevelAccelerationStructure
	{
	public:
		BottomLevelAccelerationStructure(ResourceManagerVk& resMngr, const BottomLevelAccelerationStructureDefinition& def);
		void allocate(VkBuildAccelerationStructureModeKHR buildMode, VkBuildAccelerationStructureFlagsKHR flags);
		void deallocate();
		void fillBuildInfo(VkAccelerationStructureBuildGeometryInfoKHR* infoOut, const VkAccelerationStructureBuildRangeInfoKHR*& buildRanges);
		const VkAccelerationStructureBuildSizesInfoKHR& getSizesInfo() const { return m_sizesInfo; }
		VkDeviceAddress getAccelerationStructureDeviceAddress() const { return m_accStructBuffer.deviceAddress; }
	private:
		ResourceManagerVk& m_resMngr;
		std::vector<VkAccelerationStructureGeometryKHR> m_geometries;
		std::vector<VkAccelerationStructureBuildRangeInfoKHR> m_buildRanges;
		VkBuildAccelerationStructureModeKHR m_buildMode;
		VkBuildAccelerationStructureFlagsKHR m_flags;
		VkAccelerationStructureBuildSizesInfoKHR m_sizesInfo;
		SimpleBufferAllocationUtility m_accStructBuffer;
		VkAccelerationStructureKHR m_accStruct;

	};

	class TopLevelAccelerationStructure
	{
	public:
		TopLevelAccelerationStructure(ResourceManagerVk& resMngr, const TopLevelAccelerationStructureDefinition& def);
		void allocate(VkBuildAccelerationStructureModeKHR buildMode, VkBuildAccelerationStructureFlagsKHR flags);
		void deallocate();
		void fillBuildInfo(VkAccelerationStructureBuildGeometryInfoKHR* infoOut, const VkAccelerationStructureBuildRangeInfoKHR*& buildRanges);
		const VkAccelerationStructureBuildSizesInfoKHR& getSizesInfo() const { return m_sizesInfo; }
		VkDeviceAddress getAccelerationStructureDeviceAddress() const { return m_accStructBuffer.deviceAddress; }
		VkAccelerationStructureKHR GetAccelerationStructure() const { return m_accStruct; }
		

	private:
		ResourceManagerVk& m_resMngr;
		std::vector<VkAccelerationStructureInstanceKHR> m_instances;
		VkAccelerationStructureGeometryInstancesDataKHR m_instanceStructsDef;
		VkAccelerationStructureGeometryKHR m_accStructGeometry;
		VkAccelerationStructureBuildRangeInfoKHR m_buildRange;


		VkBuildAccelerationStructureModeKHR m_buildMode;
		VkBuildAccelerationStructureFlagsKHR m_flags;
		VkAccelerationStructureBuildSizesInfoKHR m_sizesInfo;
		SimpleBufferAllocationUtility m_accStructBuffer;
		VkAccelerationStructureKHR m_accStruct;


		SimpleBufferAllocationUtility m_instancesBuildDefinitionsBuffer;
	};

}


#endif