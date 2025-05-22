#ifndef YAPT_DX12_ACCELERATIONSTRUCTUREBUILDER_H
#define YAPT_DX12_ACCELERATIONSTRUCTUREBUILDER_H

#include <Gfx/Dx12/Dx12CommonIncludes.h>
#include <Gfx/GfxTypes.h>

namespace YAPT
{
	class ResourceManagerDx12;
	struct BottomLevelAccelerationStructureDx12
	{
		std::vector<AccelerationStructureGeometryDefinition> definitions;
		RCPtr<ID3D12Resource> accelerationStructure;
		Allocation* allocation;
	};

	struct TopLevelAccelerationStructureDx12
	{
		std::vector<AccelerationStructureInstanceDefinition> definitions;
		BufferViewDx12 accelerationStructure;
		Allocation* allocation;
	};
	class AccelerationStructureBuilder
	{
	public:

		AccelerationStructureBuilder(ResourceManagerDx12& resMngr);

		void allocateBottomLevelAccelerationStructures( const BottomLevelAccelerationStructureDefinition* definitions, size_t numberOfDefinitions, BottomLevelAccelerationStructureHandle* blasArrayOut);
		void buildBottomLevelAccelerationStructures(GraphicsCommandListDx12* cmdList, BottomLevelAccelerationStructureHandle* blasArrayOut, size_t numberOfDefinitions);
		void destroyBottomLevelAccelerationStructures(BottomLevelAccelerationStructureHandle* structures, size_t numberOfStructures);

		void allocateTopLevelAccelerationStructures(const TopLevelAccelerationStructureDefinition* definitions, size_t numberOfDefinitions, TopLevelAccelerationStructureHandle* tlasArray);
		void buildTopLevelAccelerationStructures(GraphicsCommandListDx12* cmdList, TopLevelAccelerationStructureHandle* tlasArrayOut, size_t numberOfDefinitions);
		void destroyTopLevelAccelerationStructures(TopLevelAccelerationStructureHandle* structures, size_t numberOfStructures);

	private:

		ResourceManagerDx12& m_resourceMngr;
	};
}


#endif