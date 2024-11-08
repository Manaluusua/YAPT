#ifndef YAPT_DX12_ACCELERATIONSTRUCTURES_H
#define YAPT_DX12_ACCELERATIONSTRUCTURES_H

#include <Renderer/Dx12/Dx12CommonIncludes.h>
#include <Renderer/Shared/GfxTypes.h>

namespace YAPT
{
	class ResourceManagerVk;

	struct SimpleBufferAllocation
	{
		SimpleBufferAllocation()
			:buffer(VK_NULL_HANDLE),
			deviceMemory(VK_NULL_HANDLE),
			deviceAddress(NULL)
		{
		}

		void alloc(ResourceManagerVk& mngr, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlagBits memoryPropertyBits);
		void dealloc(ResourceManagerVk& mngr);
		

		VkBuffer buffer;
		VkDeviceMemory deviceMemory;
		VkDeviceAddress deviceAddress;
	};

	class BottomLevelAccelerationStructure
	{
	public:
		BottomLevelAccelerationStructure(ResourceManagerVk& resMngr, const BottomLevelAccelerationStructureDefinition& def);
		void allocate(VkBuildAccelerationStructureModeKHR buildMode, VkBuildAccelerationStructureFlagsKHR flags);
		void deallocate();
		void fillBuildInfo(VkAccelerationStructureBuildGeometryInfoKHR* infoOut, const VkAccelerationStructureBuildRangeInfoKHR*& buildRanges);
		const VkAccelerationStructureBuildSizesInfoKHR& getSizesInfo() const { return m_sizesInfo; }
		VkDeviceAddress GetAccelerationStructureDeviceAddress() const { return m_accStructBuffer.deviceAddress; }
	private:
		ResourceManagerVk& m_resMngr;
		std::vector<VkAccelerationStructureGeometryKHR> m_geometries;
		std::vector<VkAccelerationStructureBuildRangeInfoKHR> m_buildRanges;
		VkBuildAccelerationStructureModeKHR m_buildMode;
		VkBuildAccelerationStructureFlagsKHR m_flags;
		VkAccelerationStructureBuildSizesInfoKHR m_sizesInfo;
		SimpleBufferAllocation m_accStructBuffer;
		VkAccelerationStructureKHR m_accStruct;

	};

	class TopLevelAccelerationStructure
	{
	public:
		TopLevelAccelerationStructure(ResourceManagerVk& resMngr, const TopLevelAccelerationStructureDefinition& def);
		void allocate(VkBuildAccelerationStructureModeKHR buildMode, VkBuildAccelerationStructureFlagsKHR flags);
		void deallocate();
		//void fillBuildInfo(VkAccelerationStructureBuildGeometryInfoKHR* infoOut, const VkAccelerationStructureBuildRangeInfoKHR*& buildRanges);
		const VkAccelerationStructureBuildSizesInfoKHR& getSizesInfo() const { return m_sizesInfo; }
		VkDeviceAddress GetAccelerationStructureDeviceAddress() const { return m_accStructBuffer.deviceAddress; }

	private:
		ResourceManagerVk& m_resMngr;
		std::vector<VkAccelerationStructureInstanceKHR> m_instances;
		VkBuildAccelerationStructureModeKHR m_buildMode;
		VkBuildAccelerationStructureFlagsKHR m_flags;
		VkAccelerationStructureBuildSizesInfoKHR m_sizesInfo;
		SimpleBufferAllocation m_accStructBuffer;
		VkAccelerationStructureKHR m_accStruct;


		SimpleBufferAllocation m_instancesBuildDefinitionsBuffer;
	};

}


#endif