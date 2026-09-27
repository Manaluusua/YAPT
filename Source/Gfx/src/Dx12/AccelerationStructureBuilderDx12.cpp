#include <Gfx/Dx12/AccelerationStructureBuilderDx12.h>
#include <Gfx/Dx12/YaptToDx12Conversions.h>
#include <Gfx/Dx12/ResourceManagerDx12.h>
#include <Common/CommonUtilities.h>
#include <vector>

//D3D12_RAYTRACING_TRANSFORM3X4_BYTE_ALIGNMENT 
//D3D12_RAYTRACING_INSTANCE_DESC_BYTE_ALIGNMENT 
//D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT 

namespace YAPT
{
	static const D3D12_HEAP_PROPERTIES sUploadHeapProps = {
	D3D12_HEAP_TYPE_UPLOAD,
	D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
	D3D12_MEMORY_POOL_UNKNOWN,
	0,
	0,
	};

	static const D3D12_HEAP_PROPERTIES sDefaultHeapProps = {
		D3D12_HEAP_TYPE_DEFAULT,
		D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
		D3D12_MEMORY_POOL_UNKNOWN,
		0,
		0 };

	/*RCObjectPtr<ID3D12Resource> createCommittedBuffer(ID3D12Device& device, size_t size, D3D12_RESOURCE_FLAGS usageFlags, D3D12_RESOURCE_STATES initState, const D3D12_HEAP_PROPERTIES& heapProps)
	{
		

		RCObjectPtr<ID3D12Resource> buffer;

		checkForDxError(device.CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &resourceDesc, initState, nullptr, __uuidof(*buffer), buffer.asVoid()));
		return buffer;
	}*/

	Allocation* createBuffer(ResourceManagerDx12& mngr, size_t size, D3D12_HEAP_TYPE heapType, D3D12_RESOURCE_FLAGS usageFlags, D3D12_RESOURCE_STATES initState, ID3D12Resource** resOut)
	{
		D3D12_RESOURCE_DESC resourceDesc = {};
		resourceDesc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
		resourceDesc.DepthOrArraySize = 1;
		resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		resourceDesc.Flags = usageFlags;
		resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
		resourceDesc.Height = 1;
		resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		resourceDesc.MipLevels = 1;
		resourceDesc.SampleDesc.Count = 1;
		resourceDesc.SampleDesc.Quality = 0;
		resourceDesc.Width = size;

		return mngr.allocate(resourceDesc, heapType, initState, nullptr, IID_PPV_ARGS(resOut));
	}


	AccelerationStructureBuilder::AccelerationStructureBuilder(ResourceManagerDx12& resMngr)
		:m_resourceMngr(resMngr)
	{

	}

	void AccelerationStructureBuilder::allocateBottomLevelAccelerationStructures(const BottomLevelAccelerationStructureDefinition* definitions, size_t numberOfDefinitions, BottomLevelAccelerationStructureHandle* blasArrayOut)
	{
		D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS accStructureBuildFlags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;

		size_t totalNumberOfBlasDefinitions = 0;
		for (size_t blasGroupIndex = 0; blasGroupIndex < numberOfDefinitions; ++blasGroupIndex)
		{
			totalNumberOfBlasDefinitions += definitions[blasGroupIndex].geometryDefinitionCount;
		}

		std::vector<D3D12_RAYTRACING_GEOMETRY_DESC> geometryDescs;
		geometryDescs.reserve(totalNumberOfBlasDefinitions);

		std::vector<D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS> accStructureInputs;
		accStructureInputs.resize(numberOfDefinitions);

		for (size_t blasGroupIndex = 0; blasGroupIndex < numberOfDefinitions; ++blasGroupIndex)
		{
			const BottomLevelAccelerationStructureDefinition& blasDefGroup = definitions[blasGroupIndex];
			size_t geomDefCount = blasDefGroup.geometryDefinitionCount;
			const AccelerationStructureGeometryDefinition* geomDefs = blasDefGroup.geometryDefinitions;

			for (size_t geometryDefIndex = 0; geometryDefIndex < geomDefCount; ++geometryDefIndex)
			{

				const AccelerationStructureGeometryDefinition& geomDef = geomDefs[geometryDefIndex];

				BufferHandleDx12* vBuffer = static_cast<BufferHandleDx12*>(geomDef.vertexBuffer);
				BufferHandleDx12* iBuffer = static_cast<BufferHandleDx12*>(geomDef.indexBuffer);

				D3D12_GPU_VIRTUAL_ADDRESS transformAddress = NULL;
				if (geomDef.worldMatrixBuffer)
				{
					transformAddress = static_cast<BufferHandleDx12*>(geomDef.worldMatrixBuffer)->resource->GetGPUVirtualAddress();
					transformAddress += geomDef.worldMatrixBufferOffsetInBytes;
				}

				D3D12_RAYTRACING_GEOMETRY_DESC geomDesc = {};
				geomDesc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
				geomDesc.Triangles.Transform3x4 = transformAddress;
				geomDesc.Triangles.VertexBuffer.StartAddress = vBuffer->resource->GetGPUVirtualAddress() + geomDef.vertexBufferOffsetInBytes;
				geomDesc.Triangles.VertexBuffer.StrideInBytes = geomDef.vertexStrideInBytes;
				geomDesc.Triangles.VertexFormat = yaptToDx12Format(geomDef.vertexFormat);
				geomDesc.Triangles.VertexCount = static_cast<UINT>(geomDef.vertexCount);
				geomDesc.Triangles.IndexBuffer = iBuffer->resource->GetGPUVirtualAddress();
				geomDesc.Triangles.IndexCount = static_cast<UINT>(geomDef.indexCount);
				geomDesc.Triangles.IndexFormat = yaptToDx12Format(geomDef.indexFormat);
				geomDesc.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;

				geometryDescs.push_back(geomDesc);
			}

			D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS& inputs = accStructureInputs[blasGroupIndex];
			inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
			inputs.Flags = accStructureBuildFlags;
			inputs.NumDescs = (UINT)geomDefCount;
			inputs.pGeometryDescs = &geometryDescs[geometryDescs.size() - geomDefCount];
			inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;

		}

		std::vector<D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO> prebuildInfoList;
		prebuildInfoList.resize(numberOfDefinitions);

		for (size_t blasGroupIndex = 0; blasGroupIndex < numberOfDefinitions; ++blasGroupIndex)
		{
			m_resourceMngr.getDevice().GetRaytracingAccelerationStructurePrebuildInfo(accStructureInputs.data() + blasGroupIndex, prebuildInfoList.data() + blasGroupIndex);
		}

		for (size_t blasGroupIndex = 0; blasGroupIndex < numberOfDefinitions; ++blasGroupIndex)
		{
			const D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO& prebuildInfo = prebuildInfoList[blasGroupIndex];
			RCPtr<ID3D12Resource> accStructureMemory;
			Allocation* alloc =  createBuffer(m_resourceMngr, prebuildInfo.ResultDataMaxSizeInBytes, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, &accStructureMemory);

			const BottomLevelAccelerationStructureDefinition& blasDefGroup = definitions[blasGroupIndex];
			BottomLevelAccelerationStructureDx12* blasStruct = new BottomLevelAccelerationStructureDx12;
			blasStruct->accelerationStructure = accStructureMemory;
			blasStruct->definitions.assign(blasDefGroup.geometryDefinitions, blasDefGroup.geometryDefinitions + blasDefGroup.geometryDefinitionCount);
			blasStruct->allocation = alloc;
			blasArrayOut[blasGroupIndex] = blasStruct;
		}

	}
	void AccelerationStructureBuilder::buildBottomLevelAccelerationStructures(GraphicsCommandListDx12* cmdList, BottomLevelAccelerationStructureHandle* blasArray, size_t numberOfDefinitions)
	{
		D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS accStructureBuildFlags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;

		size_t totalNumberOfBlasDefinitions = 0;
		for (size_t blasGroupIndex = 0; blasGroupIndex < numberOfDefinitions; ++blasGroupIndex)
		{
			totalNumberOfBlasDefinitions += blasArray[blasGroupIndex]->definitions.size();
		}

		std::vector<D3D12_RAYTRACING_GEOMETRY_DESC> geometryDescs;
		geometryDescs.reserve(totalNumberOfBlasDefinitions);

		std::vector<D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS> accStructureInputs;
		accStructureInputs.resize(numberOfDefinitions);

		for (size_t blasGroupIndex = 0; blasGroupIndex < numberOfDefinitions; ++blasGroupIndex)
		{
			const auto& geomDefs = blasArray[blasGroupIndex]->definitions;
			size_t geomDefCount = geomDefs.size();

			for (size_t geometryDefIndex = 0; geometryDefIndex < geomDefCount; ++geometryDefIndex)
			{

				const AccelerationStructureGeometryDefinition& geomDef = geomDefs[geometryDefIndex];

				BufferHandleDx12* vBuffer = static_cast<BufferHandleDx12*>(geomDef.vertexBuffer);
				BufferHandleDx12* iBuffer = static_cast<BufferHandleDx12*>(geomDef.indexBuffer);

				D3D12_GPU_VIRTUAL_ADDRESS transformAddress = NULL;
				if (geomDef.worldMatrixBuffer)
				{
					transformAddress = static_cast<BufferHandleDx12*>(geomDef.worldMatrixBuffer)->resource->GetGPUVirtualAddress();
					transformAddress += geomDef.worldMatrixBufferOffsetInBytes;
				}

				D3D12_RAYTRACING_GEOMETRY_DESC geomDesc = {};
				geomDesc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
				geomDesc.Triangles.Transform3x4 = transformAddress;
				geomDesc.Triangles.VertexBuffer.StartAddress = vBuffer->resource->GetGPUVirtualAddress() + geomDef.vertexBufferOffsetInBytes;
				geomDesc.Triangles.VertexBuffer.StrideInBytes = geomDef.vertexStrideInBytes;
				geomDesc.Triangles.VertexFormat = yaptToDx12Format(geomDef.vertexFormat);
				geomDesc.Triangles.VertexCount = static_cast<UINT>(geomDef.vertexCount);
				geomDesc.Triangles.IndexBuffer = iBuffer->resource->GetGPUVirtualAddress() + geomDef.indexBufferOffsetInBytes;
				geomDesc.Triangles.IndexCount = static_cast<UINT>(geomDef.indexCount);
				geomDesc.Triangles.IndexFormat = yaptToDx12Format(geomDef.indexFormat);
				geomDesc.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;

				geometryDescs.push_back(geomDesc);
			}

			D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS& inputs = accStructureInputs[blasGroupIndex];
			inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
			inputs.Flags = accStructureBuildFlags;
			inputs.NumDescs = (UINT)geomDefCount;
			inputs.pGeometryDescs = &geometryDescs[geometryDescs.size() - geomDefCount];
			inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;

		}

		std::vector<D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO> prebuildInfoList;
		prebuildInfoList.resize(numberOfDefinitions);

		std::vector<D3D12_RESOURCE_BARRIER> uavBarriers;
		uavBarriers.reserve(numberOfDefinitions);

		size_t overallScrathSizeInBytes = 0;
		for (size_t blasGroupIndex = 0; blasGroupIndex < numberOfDefinitions; ++blasGroupIndex)
		{
			m_resourceMngr.getDevice().GetRaytracingAccelerationStructurePrebuildInfo(accStructureInputs.data() + blasGroupIndex, prebuildInfoList.data() + blasGroupIndex);
			overallScrathSizeInBytes += align(prebuildInfoList[blasGroupIndex].ScratchDataSizeInBytes, D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT);
		}

		overallScrathSizeInBytes += D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT;

		RCPtr<ID3D12Resource> scratch;
		Allocation* alloc = createBuffer(m_resourceMngr, overallScrathSizeInBytes, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COMMON, &scratch);
		//for now just let go of scratch, should maybe pool the scratch memory
		m_resourceMngr.addToPendingDestructionList(scratch.get());
		m_resourceMngr.addToPendingDestructionList(alloc);

		UINT64 scatchMemoryOffset = 0;

		for (size_t blasGroupIndex = 0; blasGroupIndex < numberOfDefinitions; ++blasGroupIndex)
		{
			BottomLevelAccelerationStructureDx12* blasStruct = blasArray[blasGroupIndex];
			const D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO& prebuildInfo = prebuildInfoList[blasGroupIndex];
			assert(blasStruct->accelerationStructure.get());
			ID3D12Resource* accStructureMemory = blasStruct->accelerationStructure.get();

			D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC asDesc = {};
			asDesc.Inputs = accStructureInputs[blasGroupIndex];
			asDesc.DestAccelerationStructureData = accStructureMemory->GetGPUVirtualAddress();
			asDesc.ScratchAccelerationStructureData = scratch->GetGPUVirtualAddress() + scatchMemoryOffset;
			asDesc.SourceAccelerationStructureData = NULL;

			scatchMemoryOffset += align(prebuildInfo.ScratchDataSizeInBytes, D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT);

			cmdList->BuildRaytracingAccelerationStructure(&asDesc, 0, nullptr);

			D3D12_RESOURCE_BARRIER uavBarrier = {};
			uavBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
			uavBarrier.UAV.pResource = accStructureMemory;

			uavBarriers.push_back(uavBarrier);
		}


		cmdList->ResourceBarrier(static_cast<UINT>(uavBarriers.size()), uavBarriers.data());
	}

	void AccelerationStructureBuilder::destroyBottomLevelAccelerationStructures(BottomLevelAccelerationStructureHandle* structures, size_t numberOfStructures)
	{
		for (size_t i = 0; i < numberOfStructures; ++i)
		{
			m_resourceMngr.addToPendingDestructionList(structures[i]->accelerationStructure.get());
			m_resourceMngr.addToPendingDestructionList(structures[i]->allocation);
			delete structures[i];
		}
	}


	void AccelerationStructureBuilder::allocateTopLevelAccelerationStructures(const TopLevelAccelerationStructureDefinition* definitions, size_t numberOfDefinitions, TopLevelAccelerationStructureHandle* tlasArray)
	{
		D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS accStructureBuildFlags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;

		size_t totalNumberOfinstanceDefinitions = 0;
		for (size_t tlasGroupIndex = 0; tlasGroupIndex < numberOfDefinitions; ++tlasGroupIndex)
		{
			totalNumberOfinstanceDefinitions += definitions[tlasGroupIndex].instanceDefinitionCount;
		}

		std::vector<D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS> accStructureInputs;
		accStructureInputs.resize(numberOfDefinitions);

		for (size_t tlasGroupIndex = 0; tlasGroupIndex < numberOfDefinitions; ++tlasGroupIndex)
		{

			const TopLevelAccelerationStructureDefinition& tlasDefGroup = definitions[tlasGroupIndex];
			size_t instanceDefCount = tlasDefGroup.instanceDefinitionCount;
			const AccelerationStructureInstanceDefinition* instanceDefs = tlasDefGroup.instanceDefinitions;

			D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS& inputs = accStructureInputs[tlasGroupIndex];
			inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
			inputs.Flags = accStructureBuildFlags;
			inputs.NumDescs = static_cast<UINT>(instanceDefCount);

			inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
		}

		std::vector<D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO> prebuildInfoList;
		prebuildInfoList.resize(numberOfDefinitions);

		for (size_t tlasGroupIndex = 0; tlasGroupIndex < numberOfDefinitions; ++tlasGroupIndex)
		{
			m_resourceMngr.getDevice().GetRaytracingAccelerationStructurePrebuildInfo(accStructureInputs.data() + tlasGroupIndex, prebuildInfoList.data() + tlasGroupIndex);
		}

		for (size_t tlasGroupIndex = 0; tlasGroupIndex < numberOfDefinitions; ++tlasGroupIndex)
		{
			const D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO& prebuildInfo = prebuildInfoList[tlasGroupIndex];
			const TopLevelAccelerationStructureDefinition& tlasDefGroup = definitions[tlasGroupIndex];

			RCPtr<ID3D12Resource> accStructureMemory;
			Allocation* alloc = createBuffer(m_resourceMngr, prebuildInfo.ResultDataMaxSizeInBytes, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, &accStructureMemory);


			TopLevelAccelerationStructureDx12* tlasStruct = new TopLevelAccelerationStructureDx12;
			tlasStruct->accelerationStructure.resource = accStructureMemory;
			tlasStruct->definitions.assign(tlasDefGroup.instanceDefinitions, tlasDefGroup.instanceDefinitions + tlasDefGroup.instanceDefinitionCount);
			tlasStruct->allocation = alloc;

			BufferViewDesc& bufferViewDesc = tlasStruct->accelerationStructure.desc;
			bufferViewDesc.offsetInBytes = 0;
			bufferViewDesc.nonStructuredFormat = ResourceFormat::UNKNOWN;
			bufferViewDesc.sizeInBytes = prebuildInfo.ResultDataMaxSizeInBytes;
			bufferViewDesc.structureStrideInBytes = prebuildInfo.ResultDataMaxSizeInBytes;

			tlasArray[tlasGroupIndex] = tlasStruct;
		}
	}
	void AccelerationStructureBuilder::buildTopLevelAccelerationStructures(GraphicsCommandListDx12* cmdList, TopLevelAccelerationStructureHandle* tlasArray, size_t numberOfDefinitions)
	{
		D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS accStructureBuildFlags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;

		size_t totalNumberOfinstanceDefinitions = 0;
		for (size_t tlasGroupIndex = 0; tlasGroupIndex < numberOfDefinitions; ++tlasGroupIndex)
		{
			totalNumberOfinstanceDefinitions += tlasArray[tlasGroupIndex]->definitions.size();
		}


		std::vector<D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS> accStructureInputs;
		accStructureInputs.resize(numberOfDefinitions);

		//somewhat silly but make sure the D3D12_RAYTRACING_INSTANCE_DESC has not changed and is aligned to D3D12_RAYTRACING_INSTANCE_DESCS_BYTE_ALIGNMENT
		assert(isAligned(sizeof(D3D12_RAYTRACING_INSTANCE_DESC), (size_t)D3D12_RAYTRACING_INSTANCE_DESCS_BYTE_ALIGNMENT));

		if (totalNumberOfinstanceDefinitions > 0)
		{

			RCPtr<ID3D12Resource> instanceData;
			Allocation* alloc = createBuffer(m_resourceMngr, sizeof(D3D12_RAYTRACING_INSTANCE_DESC) * totalNumberOfinstanceDefinitions, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_COMMON, &instanceData);

			m_resourceMngr.addToPendingDestructionList(instanceData);
			m_resourceMngr.addToPendingDestructionList(alloc);

			D3D12_RAYTRACING_INSTANCE_DESC* instanceDataDstPtr;
			checkForDxError(instanceData->Map(0, nullptr, (void**)&instanceDataDstPtr));

			UINT64 currentInstanceOffset = 0;

			for (size_t tlasGroupIndex = 0; tlasGroupIndex < numberOfDefinitions; ++tlasGroupIndex)
			{
				const auto& instanceDefs = tlasArray[tlasGroupIndex]->definitions;
				size_t instanceDefCount = instanceDefs.size();


				D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS& inputs = accStructureInputs[tlasGroupIndex];
				inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
				inputs.Flags = accStructureBuildFlags;
				inputs.NumDescs = static_cast<UINT>(instanceDefCount);
				inputs.InstanceDescs = instanceData->GetGPUVirtualAddress() + currentInstanceOffset;
				inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;


				for (size_t instanceInd = 0; instanceInd < instanceDefCount; ++instanceInd)
				{
					const AccelerationStructureInstanceDefinition& instanceDef = instanceDefs[instanceInd];

					instanceDataDstPtr->InstanceID = instanceDef.instanceID;
					instanceDataDstPtr->InstanceContributionToHitGroupIndex = instanceDef.hitGroupShaderTableOffset;
					instanceDataDstPtr->Flags = instanceDef.forceNonOpaque ? D3D12_RAYTRACING_INSTANCE_FLAG_FORCE_NON_OPAQUE : D3D12_RAYTRACING_INSTANCE_FLAG_NONE;
					memcpy(instanceDataDstPtr->Transform, instanceDef.instanceToWorld, sizeof(instanceDataDstPtr->Transform));
					instanceDataDstPtr->AccelerationStructure = instanceDef.blas->accelerationStructure->GetGPUVirtualAddress();
					instanceDataDstPtr->InstanceMask = instanceDef.instanceMask;

					++instanceDataDstPtr;
					++currentInstanceOffset;
				}
			}

			instanceData->Unmap(0, nullptr);
		}
		

		std::vector<D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO> prebuildInfoList;
		prebuildInfoList.resize(numberOfDefinitions);

		std::vector<D3D12_RESOURCE_BARRIER> uavBarriers;
		uavBarriers.reserve(numberOfDefinitions);

		size_t overallScrathSizeInBytes = 0;
		for (size_t tlasGroupIndex = 0; tlasGroupIndex < numberOfDefinitions; ++tlasGroupIndex)
		{
			m_resourceMngr.getDevice().GetRaytracingAccelerationStructurePrebuildInfo(accStructureInputs.data() + tlasGroupIndex, prebuildInfoList.data() + tlasGroupIndex);
			overallScrathSizeInBytes += prebuildInfoList[tlasGroupIndex].ScratchDataSizeInBytes;
		}
		RCPtr<ID3D12Resource> scratch;
		Allocation* alloc = createBuffer(m_resourceMngr, overallScrathSizeInBytes, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COMMON, &scratch);

		m_resourceMngr.addToPendingDestructionList(scratch.get());
		m_resourceMngr.addToPendingDestructionList(alloc);
		

		UINT64 scratchMemoryOffset = 0;

		for (size_t tlasGroupIndex = 0; tlasGroupIndex < numberOfDefinitions; ++tlasGroupIndex)
		{
			const D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO& prebuildInfo = prebuildInfoList[tlasGroupIndex];
			TopLevelAccelerationStructureDx12* tlasStruct = tlasArray[tlasGroupIndex];
			assert(tlasStruct != nullptr);
			ID3D12Resource* accStructureMemory = tlasStruct->accelerationStructure.resource.get();
			assert(accStructureMemory != nullptr);

			D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC asDesc = {};
			asDesc.Inputs = accStructureInputs[tlasGroupIndex];
			asDesc.DestAccelerationStructureData = accStructureMemory->GetGPUVirtualAddress();
			asDesc.ScratchAccelerationStructureData = scratch->GetGPUVirtualAddress() + scratchMemoryOffset;
			asDesc.SourceAccelerationStructureData = NULL;

			scratchMemoryOffset += prebuildInfo.ScratchDataSizeInBytes;

			cmdList->BuildRaytracingAccelerationStructure(&asDesc, 0, nullptr);

			D3D12_RESOURCE_BARRIER uavBarrier = {};
			uavBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
			uavBarrier.UAV.pResource = accStructureMemory;

			uavBarriers.push_back(uavBarrier);

			
			tlasStruct->accelerationStructure.resource = accStructureMemory;
			BufferViewDesc& bufferViewDesc = tlasStruct->accelerationStructure.desc;
			bufferViewDesc.offsetInBytes = 0;
			bufferViewDesc.nonStructuredFormat = ResourceFormat::UNKNOWN;
			bufferViewDesc.sizeInBytes = prebuildInfo.ResultDataMaxSizeInBytes;
			bufferViewDesc.structureStrideInBytes = prebuildInfo.ResultDataMaxSizeInBytes;
		}

		cmdList->ResourceBarrier(static_cast<UINT>(uavBarriers.size()), uavBarriers.data());
	}



	void AccelerationStructureBuilder::destroyTopLevelAccelerationStructures(TopLevelAccelerationStructureHandle* structures, size_t numberOfStructures)
	{
		for (size_t i = 0; i < numberOfStructures; ++i)
		{
			m_resourceMngr.addToPendingDestructionList(structures[i]->accelerationStructure.resource.get());
			m_resourceMngr.addToPendingDestructionList(structures[i]->allocation);
			delete structures[i];
		}
	}

}