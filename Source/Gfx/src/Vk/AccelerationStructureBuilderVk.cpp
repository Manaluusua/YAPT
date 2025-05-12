#include <Gfx/Vk/AccelerationStructureBuilderVk.h>
#include <Common/CommonUtilities.h>
#include <vector>
#include <Gfx/Vk/AccelerationStructuresVk.h>
#include <Gfx/Vk/ResourceManagerVk.h>

namespace YAPT
{
	const uint32_t DEFAULT_SCRATCH_SIZE = 1024 * 1024 * 64;
	AccelerationStructureBuilder::AccelerationStructureBuilder(ResourceManagerVk& resMngr)
		:m_resourceMngr(resMngr)
	{
		allocateScratch(DEFAULT_SCRATCH_SIZE);
	}

	AccelerationStructureBuilder::~AccelerationStructureBuilder()
	{
		freeScratch();
	}

	void AccelerationStructureBuilder::allocateBottomLevelAccelerationStructures(const BottomLevelAccelerationStructureDefinition* definitions, BottomLevelAccelerationStructureHandle* blasArrayOut, size_t numberOfDefinitions)
	{

		for (size_t blasInd = 0; blasInd < numberOfDefinitions; ++blasInd)
		{
			const BottomLevelAccelerationStructureDefinition& def = definitions[blasInd];
			BottomLevelAccelerationStructure* blas = new BottomLevelAccelerationStructure(m_resourceMngr, def);
			blas->allocate(VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR, VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR);
			blasArrayOut[blasInd] = blas;
		}


	}
	void AccelerationStructureBuilder::buildBottomLevelAccelerationStructures(VkCommandBuffer cmdList, BottomLevelAccelerationStructureHandle* blasArray, size_t numberOfStructures)
	{
		std::vector<VkAccelerationStructureBuildGeometryInfoKHR> buildInfo;
		std::vector<const VkAccelerationStructureBuildRangeInfoKHR*> buildRanges;
		buildInfo.resize(numberOfStructures);
		buildRanges.resize(numberOfStructures);

		//barrier between flushes
		VkMemoryBarrier barrier = {};
		barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
		barrier.srcAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;
		barrier.dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR;


		auto flush = [&](size_t entryOffset, size_t count)
		{
			m_resourceMngr.getVkExtFuncs().vkCmdBuildAccelerationStructuresKHR(cmdList, (uint32_t)count, buildInfo.data() + entryOffset, buildRanges.data() + entryOffset);
			vkCmdPipelineBarrier(cmdList, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR, 0, 1, &barrier, 0, nullptr, 0, nullptr);
		};
		//ensure scratch
		for (size_t i = 0; i < numberOfStructures; ++i)
		{
			VkDeviceSize scratchSize = align(blasArray[i]->getSizesInfo().buildScratchSize, m_scratchBuffer.memoryRequirements.alignment);
			ensureScratch(scratchSize);
		}

		uint64_t currentScratchAddress = m_scratchBuffer.deviceAddress;
		VkDeviceSize usedScratchMemory = 0;
		size_t batchOffset = 0;
		for (size_t i = 0; i < numberOfStructures; ++i)
		{
			VkDeviceSize scratchSize = align(blasArray[i]->getSizesInfo().buildScratchSize, m_scratchBuffer.memoryRequirements.alignment);

			if (usedScratchMemory + scratchSize >= m_scratchBuffer.memoryRequirements.size)
			{
				//flush
				flush(batchOffset, i - batchOffset);
				batchOffset = i;
				
			}
			const VkAccelerationStructureBuildRangeInfoKHR*& buildRange = *(buildRanges.data() + i);
			blasArray[i]->fillBuildInfo(buildInfo.data() + i, buildRange);
			buildInfo[i].scratchData.deviceAddress = currentScratchAddress + usedScratchMemory;
			usedScratchMemory += scratchSize;
		}

		
		flush(batchOffset, numberOfStructures - batchOffset);

	}


	void AccelerationStructureBuilder::destroyBottomLevelAccelerationStructures(BottomLevelAccelerationStructureHandle* structures, size_t numberOfStructures)
	{
		for (size_t blasInd = 0; blasInd < numberOfStructures; ++blasInd)
		{
			structures[blasInd]->deallocate();
			delete structures[blasInd];
		}
	}


	void AccelerationStructureBuilder::allocateTopLevelAccelerationStructures(const TopLevelAccelerationStructureDefinition* definitions, TopLevelAccelerationStructureHandle* tlasArrayOut, size_t numberOfDefinitions)
	{
		for (size_t blasInd = 0; blasInd < numberOfDefinitions; ++blasInd)
		{
			const TopLevelAccelerationStructureDefinition& def = definitions[blasInd];
			TopLevelAccelerationStructure* tlas = new TopLevelAccelerationStructure(m_resourceMngr, def);
			tlas->allocate(VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR, VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR);
			tlasArrayOut[blasInd] = tlas;

		}
	}


	void AccelerationStructureBuilder::buildTopLevelAccelerationStructures(VkCommandBuffer cmdList, TopLevelAccelerationStructureHandle* tlasArray, size_t numberOfStructures)
	{
		std::vector<VkAccelerationStructureBuildGeometryInfoKHR> buildInfo;
		std::vector<const VkAccelerationStructureBuildRangeInfoKHR*> buildRanges;
		buildInfo.resize(numberOfStructures);
		buildRanges.resize(numberOfStructures);

		//barrier between flushes
		VkMemoryBarrier barrier = {};
		barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
		barrier.srcAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;
		barrier.dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR;


		auto flush = [&](size_t entryOffset, size_t count)
		{
			m_resourceMngr.getVkExtFuncs().vkCmdBuildAccelerationStructuresKHR(cmdList, (uint32_t)count, buildInfo.data() + entryOffset, buildRanges.data() + entryOffset);
			vkCmdPipelineBarrier(cmdList, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR, 0, 1, &barrier, 0, nullptr, 0, nullptr);
		};
		//ensure scratch
		for (size_t i = 0; i < numberOfStructures; ++i)
		{
			VkDeviceSize scratchSize = align(tlasArray[i]->getSizesInfo().buildScratchSize, m_scratchBuffer.memoryRequirements.alignment);
			ensureScratch(scratchSize);
		}

		uint64_t currentScratchAddress = m_scratchBuffer.deviceAddress;
		VkDeviceSize usedScratchMemory = 0;
		size_t batchOffset = 0;
		for (size_t i = 0; i < numberOfStructures; ++i)
		{
			VkDeviceSize scratchSize = align(tlasArray[i]->getSizesInfo().buildScratchSize, m_scratchBuffer.memoryRequirements.alignment);

			if (usedScratchMemory + scratchSize >= m_scratchBuffer.memoryRequirements.size)
			{
				//flush
				flush(batchOffset, i - batchOffset);
				batchOffset = i;

			}
			const VkAccelerationStructureBuildRangeInfoKHR*& buildRange = *(buildRanges.data() + i);
			tlasArray[i]->fillBuildInfo(buildInfo.data() + i, buildRange);
			buildInfo[i].scratchData.deviceAddress = currentScratchAddress + usedScratchMemory;
			usedScratchMemory += scratchSize;
		}


		flush(batchOffset, numberOfStructures - batchOffset);
	}

	void AccelerationStructureBuilder::destroyTopLevelAccelerationStructures(TopLevelAccelerationStructureHandle* structures, size_t numberOfStructures)
	{
		for (size_t tlasInd = 0; tlasInd < numberOfStructures; ++tlasInd)
		{
			structures[tlasInd]->deallocate();
			delete structures[tlasInd];
		}
	}

	void AccelerationStructureBuilder::freeScratch()
	{
		if (m_scratchBuffer.buffer != VK_NULL_HANDLE)
		{
			m_scratchBuffer.dealloc(m_resourceMngr);
			m_scratchBuffer.memoryRequirements.size = 0;
		}
		
	}
	void AccelerationStructureBuilder::allocateScratch(VkDeviceSize sizeInBytes)
	{
		m_scratchBuffer.alloc(m_resourceMngr, sizeInBytes, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		
	}
	void AccelerationStructureBuilder::ensureScratch(VkDeviceSize sizeInBytes)
	{
		if (m_scratchBuffer.memoryRequirements.size < sizeInBytes)
		{
			freeScratch();
			allocateScratch(sizeInBytes);
		}
	}

}