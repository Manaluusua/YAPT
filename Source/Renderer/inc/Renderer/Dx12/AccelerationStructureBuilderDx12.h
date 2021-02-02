#ifndef YAPT_DX12_ACCELERATIONSTRUCTUREBUILDER_H
#define YAPT_DX12_ACCELERATIONSTRUCTUREBUILDER_H

#include <Renderer/Dx12/Dx12CommonIncludes.h>
#include <Renderer/Shared/GfxTypes.h>

namespace YAPT
{
	class ResourceManagerDx12;
	class AccelerationStructureBuilder
	{
	public:

		AccelerationStructureBuilder(ResourceManagerDx12& resMngr);

		

		void allocateBottomLevelAccelerationStructures( const BottomLevelAccelerationStructureDefinition* definitions, size_t numberOfDefinitions, BottomLevelAccelerationStructureHandle* blasArrayOut);
		void buildBottomLevelAccelerationStructures(GraphicsCommandListDx12* cmdList, const BottomLevelAccelerationStructureDefinition* definitions, size_t numberOfDefinitions, BottomLevelAccelerationStructureHandle* blasArrayOut);
		void destroyBottomLevelAccelerationStructures(BottomLevelAccelerationStructureHandle* structures, size_t numberOfStructures);

		void allocateTopLevelAccelerationStructures(const TopLevelAccelerationStructureDefinition* definitions, size_t numberOfDefinitions, TopLevelAccelerationStructureHandle* tlasArray);
		void buildTopLevelAccelerationStructures(GraphicsCommandListDx12* cmdList, const TopLevelAccelerationStructureDefinition* definitions, size_t numberOfDefinitions, TopLevelAccelerationStructureHandle* tlasArrayOut);
		void destroyTopLevelAccelerationStructures(TopLevelAccelerationStructureHandle* structures, size_t numberOfStructures);

	private:

		ResourceManagerDx12& m_resourceMngr;
	};
}


#endif