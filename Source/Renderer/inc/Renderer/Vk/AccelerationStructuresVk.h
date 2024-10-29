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
		void allocate();
		void build();

	private:
		ResourceManagerVk& m_resMngr;
		std::vector<VkAccelerationStructureGeometryKHR> m_geometries;
		std::vector<VkAccelerationStructureBuildRangeInfoKHR> m_buildRanges;
	};

	class TopLevelAccelerationStructure
	{

	};

}


#endif