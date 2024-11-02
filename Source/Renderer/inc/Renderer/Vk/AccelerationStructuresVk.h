#ifndef YAPT_DX12_ACCELERATIONSTRUCTURES_H
#define YAPT_DX12_ACCELERATIONSTRUCTURES_H

#include <Renderer/Dx12/Dx12CommonIncludes.h>
#include <Renderer/Shared/GfxTypes.h>

namespace YAPT
{
	class ResourceManagerVk;
	class BottomLevelAccelerationStructure
	{
	public:
		BottomLevelAccelerationStructure(ResourceManagerVk& resMngr, const BottomLevelAccelerationStructureDefinition& def);
		void allocate(VkBuildAccelerationStructureModeKHR buildMode, VkBuildAccelerationStructureFlagsKHR flags);
		void build(VkBuildAccelerationStructureFlagsKHR flags);

	private:
		ResourceManagerVk& m_resMngr;
		std::vector<VkAccelerationStructureGeometryKHR> m_geometries;
		std::vector<VkAccelerationStructureBuildRangeInfoKHR> m_buildRanges;
		VkAccelerationStructureBuildSizesInfoKHR m_sizesInfo;
		VkBuffer m_buffer;
		VkDeviceMemory m_deviceMemory;
	};

	class TopLevelAccelerationStructure
	{

	};

}


#endif