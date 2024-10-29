#include <Renderer/Vk/AccelerationStructureBuilderVk.h>
#include <Common/CommonUtilities.h>
#include <vector>
#include <Renderer/Vk/AccelerationStructuresVk.h>
#include <Renderer/Vk/ResourceManagerVk.h>

namespace YAPT
{

	AccelerationStructureBuilder::AccelerationStructureBuilder(ResourceManagerVk& resMngr)
		:m_resourceMngr(resMngr)
	{

	}

	void AccelerationStructureBuilder::allocateBottomLevelAccelerationStructures(const BottomLevelAccelerationStructureDefinition* definitions, size_t numberOfDefinitions, BottomLevelAccelerationStructureHandle* blasArrayOut)
	{

		for (size_t blasInd = 0; blasInd < numberOfDefinitions; ++blasInd)
		{
			const BottomLevelAccelerationStructureDefinition& def = definitions[blasInd];
			BottomLevelAccelerationStructure* blas = new BottomLevelAccelerationStructure(m_resourceMngr, def);
			blasArrayOut[blasInd] = blas;
			
		}


	}
	void AccelerationStructureBuilder::buildBottomLevelAccelerationStructures(VkCommandBuffer cmdList, const BottomLevelAccelerationStructureDefinition* definitions, size_t numberOfDefinitions, BottomLevelAccelerationStructureHandle* blasArray)
	{
		assert(!"NOT IMPLEMENTED!");
	}


	void AccelerationStructureBuilder::destroyBottomLevelAccelerationStructures(BottomLevelAccelerationStructureHandle* structures, size_t numberOfStructures)
	{
		assert(!"NOT IMPLEMENTED!");
	}


	void AccelerationStructureBuilder::allocateTopLevelAccelerationStructures(const TopLevelAccelerationStructureDefinition* definitions, size_t numberOfDefinitions, TopLevelAccelerationStructureHandle* tlasArray)
	{
		assert(!"NOT IMPLEMENTED!");
	}


	void AccelerationStructureBuilder::buildTopLevelAccelerationStructures(VkCommandBuffer cmdList, const TopLevelAccelerationStructureDefinition* definitions, size_t numberOfDefinitions, TopLevelAccelerationStructureHandle* tlasArrayOut)
	{
		assert(!"NOT IMPLEMENTED!");
	}

	void AccelerationStructureBuilder::destroyTopLevelAccelerationStructures(TopLevelAccelerationStructureHandle* structures, size_t numberOfStructures)
	{
		assert(!"NOT IMPLEMENTED!");
	}

}