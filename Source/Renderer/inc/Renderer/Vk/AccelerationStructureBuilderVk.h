#ifndef YAPT_DX12_ACCELERATIONSTRUCTUREBUILDER_H
#define YAPT_DX12_ACCELERATIONSTRUCTUREBUILDER_H

#include <Renderer/Dx12/Dx12CommonIncludes.h>
#include <Renderer/Shared/GfxTypes.h>
#include <Renderer/Vk/SimpleBufferAllocationUtility.h>

namespace YAPT
{
	class ResourceManagerVk;
	class AccelerationStructureBuilder
	{
	public:

		AccelerationStructureBuilder(ResourceManagerVk& resMngr);
		~AccelerationStructureBuilder();

		void allocateBottomLevelAccelerationStructures( const BottomLevelAccelerationStructureDefinition* definitions,  BottomLevelAccelerationStructureHandle* blasArrayOut, size_t numberOfDefinitions);
		void buildBottomLevelAccelerationStructures(VkCommandBuffer cmdList,  BottomLevelAccelerationStructureHandle* blasArrayOut, size_t numberOfStructures);
		void destroyBottomLevelAccelerationStructures(BottomLevelAccelerationStructureHandle* structures, size_t numberOfStructures);

		void allocateTopLevelAccelerationStructures(const TopLevelAccelerationStructureDefinition* definitions, TopLevelAccelerationStructureHandle* tlasArrayOut, size_t numberOfDefinitions);
		void buildTopLevelAccelerationStructures(VkCommandBuffer cmdList, TopLevelAccelerationStructureHandle* tlasArrayOut, size_t numberOfStructures);
		void destroyTopLevelAccelerationStructures(TopLevelAccelerationStructureHandle* structures, size_t numberOfStructures);

	private:

		void freeScratch();
		void allocateScratch(VkDeviceSize sizeInBytes);
		void ensureScratch(VkDeviceSize sizeInBytes);

		ResourceManagerVk& m_resourceMngr;
		SimpleBufferAllocationUtility m_scratchBuffer;
		VkMemoryRequirements m_scratchMemoryReqs;
	};
}


#endif