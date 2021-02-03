#include <Renderer/Dx12/ResourceManagerDx12.h>
#include <Renderer/Dx12/DescriptorHeapDx12.h>
#include <Renderer/Dx12/ResourceAllocationPoolDx12.h>
#include <Renderer/Dx12/ShaderUtilityDx12.h>
#include <Renderer/Dx12/Dx12MiscUtils.h>
#include <Renderer/Dx12/d3dx12.h>
#include <Renderer/Dx12/UploadHelperDx12.h>
#include <Renderer/Dx12/DescriptorHeapAllocatorDx12.h>
#include <Renderer/Dx12/DescriptorSetPoolDx12.h>
#include <Common/CommonWindowsUtility.h>
#include <Renderer/Shared/Utility/DXCUtility.h>
#include <assert.h>


namespace YAPT
{
	ResourceManagerDx12::ResourceManagerDx12(ID3D12Device5& device, size_t pipelineLength)
		:m_device(device),
		m_preFrameUploads(nullptr),
		m_duringFrameUploads(nullptr),
		m_pipelineLength(pipelineLength)
	{
		
	}
	ResourceManagerDx12::~ResourceManagerDx12()
	{
		deinitialize();
		
	}

	bool ResourceManagerDx12::initialize(SubmissionThreadDx12* submissionThread, size_t assetUploadHeapSize, size_t renderUploadHeapSize)
	{
		m_submissionThread = submissionThread;

		m_pendingDestroyedObjects.resize(m_pipelineLength + 1);
		m_destroyObjectsIndex = 0;

		m_preFrameUploads = new UploadHelperDx12(*this, submissionThread, assetUploadHeapSize, m_pipelineLength);
		m_duringFrameUploads = new UploadHelperDx12(*this, submissionThread, renderUploadHeapSize, m_pipelineLength);

		m_nonSamplerDescHeapAllocator = new DescriptorHeapAllocatorDx12(*this, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, DEFAULT_DESCRIPTORHEAP_SIZE_NONSAMPLER);
		m_samplerDescHeapAllocator = new DescriptorHeapAllocatorDx12(*this, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, DEFAULT_DESCRIPTORHEAP_SIZE_SAMPLER);

		return true;
	}

	void ResourceManagerDx12::deinitialize()
	{
		if (m_nonSamplerDescHeapAllocator)
		{
			delete m_nonSamplerDescHeapAllocator;
			m_nonSamplerDescHeapAllocator = nullptr;
		}

		if (m_samplerDescHeapAllocator)
		{
			delete m_samplerDescHeapAllocator;
			m_samplerDescHeapAllocator = nullptr;
		}
		
		if (m_preFrameUploads)
		{
			delete m_preFrameUploads;
			m_preFrameUploads = nullptr;
		}
		if (m_duringFrameUploads)
		{
			delete m_duringFrameUploads;
			m_duringFrameUploads = nullptr;
		}

		m_pendingDestroyedObjects.clear();
	}

	DescriptorHeapDx12* ResourceManagerDx12::createDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE type, size_t descriptorCount)
	{
		D3D12_DESCRIPTOR_HEAP_FLAGS flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		if (type == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV || type == D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER)
		{
			flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		}

		return new DescriptorHeapDx12(*this, type, descriptorCount, flags);
	}

	RCPtr<ID3D12Heap> ResourceManagerDx12::createResourceHeap(D3D12_HEAP_TYPE type, size_t sizeInBytes, D3D12_HEAP_FLAGS flags, size_t alignment)
	{
		D3D12_HEAP_DESC heapDesc;
		heapDesc.Properties.Type = type;
		heapDesc.Properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		heapDesc.Properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		heapDesc.Properties.CreationNodeMask = 0;
		heapDesc.Properties.VisibleNodeMask = 0;
		heapDesc.SizeInBytes = (UINT64)sizeInBytes;
		heapDesc.Flags = flags;
		heapDesc.Alignment = (UINT64)alignment;
		
		RCPtr<ID3D12Heap> heap;
		m_device.CreateHeap(&heapDesc, IID_PPV_ARGS(&heap));
		return heap;
	}


	void ResourceManagerDx12::releaseDescriptorHeap(DescriptorHeapDx12* heap)
	{
		ID3D12Object* r = heap->m_descHeap.get();
		addToPendingDestructionList(&r, 1);
		delete heap;
	}

	DescriptorSetPoolDx12* ResourceManagerDx12::createDescriptorSetPool(const DescriptorSetLayoutDx12* layout, size_t numberOfDescriptorSets)
	{
		return new DescriptorSetPoolDx12(*this, layout, m_nonSamplerDescHeapAllocator, m_samplerDescHeapAllocator, numberOfDescriptorSets);
	}
	void ResourceManagerDx12::destroyDescriptorSetPool(DescriptorSetPoolDx12* pool)
	{
		delete pool;
	}

	void ResourceManagerDx12::upload(BufferHandleDx12* handle, size_t offsetInBytes, size_t sizeInBytes, const void* data, GpuUploadStage heapType)
	{
		if (handle->heapType == D3D12_HEAP_TYPE_DEFAULT)
		{
			copyViaUploadHeap(handle->resource, offsetInBytes, sizeInBytes, data, heapType);
		}
		else if (handle->heapType == D3D12_HEAP_TYPE_UPLOAD)
		{
			uint8_t* ptr = (uint8_t*)handle->map();
			assert(ptr != nullptr);
			memcpy(ptr + offsetInBytes, data, sizeInBytes);
			handle->unmap();
		}
		else
		{
			YAPT_LOG_FATAL_ERROR("Map for given heaptype not supported");
		}
	}
	void* ResourceManagerDx12::map(BufferHandleDx12* handle, size_t offsetInBytes, size_t sizeInBytes, GpuUploadStage heapType)
	{
		if (handle->heapType == D3D12_HEAP_TYPE_DEFAULT)
		{
			return mapCopyRangeFromUploadHeap(handle->resource, offsetInBytes, sizeInBytes, heapType);
		}
		else if (handle->heapType == D3D12_HEAP_TYPE_UPLOAD)
		{
			uint8_t* ptr = (uint8_t*)handle->map();
			return ptr + offsetInBytes;
		}
		else
		{
			YAPT_LOG_FATAL_ERROR("Map for given heaptype not yet implemented");
			return nullptr;
		}
	}

	void ResourceManagerDx12::unmap(BufferHandleDx12* handle)
	{
		if (handle->heapType == D3D12_HEAP_TYPE_UPLOAD)
		{
			handle->unmap();
		}
	}

	void ResourceManagerDx12::upload(TextureHandle image, size_t arraySliceOffset, size_t arraySliceCount, size_t mipOffset, size_t mipCount, const TextureDataDefinition* textureDataDefinitions, GpuUploadStage heapType)
	{
		if (image->heapType == D3D12_HEAP_TYPE_DEFAULT)
		{
			copyViaUploadHeap(image->resource, image->textureDesc, arraySliceOffset, arraySliceCount, mipOffset, mipCount, textureDataDefinitions, heapType);
		}
		else
		{
			YAPT_LOG_FATAL_ERROR("Map for given heaptype not yet implemented");
		}
	}


	void* ResourceManagerDx12::mapCopyRangeFromUploadHeap(ID3D12Resource* buffer, size_t offsetInBytes, size_t sizeInBytes, GpuUploadStage heapType)
	{
		switch (heapType)
		{
		case YAPT::GpuUploadStage::BEFORE_RENDER:
		{
			return m_preFrameUploads->mapCopyRangeForBufferData(buffer, offsetInBytes, sizeInBytes);
			break;
		}
		case YAPT::GpuUploadStage::DURING_RENDER:
		{
			return m_duringFrameUploads->mapCopyRangeForBufferData(buffer, offsetInBytes, sizeInBytes);
			break;
		}
		default:
			assert(!"unknown upload stage");
			break;
		}
		return nullptr;
	}

	void ResourceManagerDx12::copyViaUploadHeap(ID3D12Resource* buffer, size_t offsetInBytes, size_t sizeInBytes, const void* data, GpuUploadStage heapType)
	{
		switch (heapType)
		{
		case YAPT::GpuUploadStage::BEFORE_RENDER:
		{
			m_preFrameUploads->uploadDataForBuffer(buffer, offsetInBytes, sizeInBytes, data);
			break;
		}
		case YAPT::GpuUploadStage::DURING_RENDER:
		{
			m_duringFrameUploads->uploadDataForBuffer(buffer, offsetInBytes, sizeInBytes, data);
			break;
		}
		default:
			assert(!"unknown upload stage");
			break;
		}
	}


	void ResourceManagerDx12::copyViaUploadHeap(ID3D12Resource* texture, const D3D12_RESOURCE_DESC& resourceDesc, size_t arraySliceOffset, size_t arraySliceCount, size_t mipOffset, size_t mipCount, const TextureDataDefinition* textureDataDefinitions, GpuUploadStage heapType)
	{
		switch (heapType)
		{
		case YAPT::GpuUploadStage::BEFORE_RENDER:
		{
			m_preFrameUploads->uploadDataForTexture(texture, resourceDesc, arraySliceOffset, arraySliceCount, mipOffset, mipCount, textureDataDefinitions);
			break;
		}
		case YAPT::GpuUploadStage::DURING_RENDER:
		{
			m_duringFrameUploads->uploadDataForTexture(texture, resourceDesc, arraySliceOffset, arraySliceCount, mipOffset, mipCount,  textureDataDefinitions);
			break;
		}
		default:
			assert(!"unknown upload stage");
			break;
		}
	}

	void ResourceManagerDx12::addToPendingDestructionList(ID3D12Object** objects, size_t objCount)
	{
		//This can cause high contention, should consider less locking approach later on...
		std::unique_lock<std::mutex> lock(m_destroyObjectsMutex);
		for (size_t i = 0; i < objCount; ++i)
		{
			m_pendingDestroyedObjects[m_destroyObjectsIndex].push_back(objects[i]);
		}
		
	}

	size_t ResourceManagerDx12::getFrameNumber() const
	{ 
		return m_frameNumber; 
	}


	DescriptorHeapDx12* ResourceManagerDx12::getNonSamplerDescHeap() const
	{ 
		return m_nonSamplerDescHeapAllocator->getHeap(); 
	}
	DescriptorHeapDx12* ResourceManagerDx12::getSamplerDescHeap() const
	{ 
		return m_samplerDescHeapAllocator->getHeap();
	}

	uint32_t ResourceManagerDx12::getBufferMinimumAlignment(ResourceUsage resourceUsage)
	{
		int maxAlignment = D3D12_RAW_UAV_SRV_BYTE_ALIGNMENT;
		if ((resourceUsage & (RESOURCE_USAGE_UNIFORM_BUFFER)) != 0)
		{
			maxAlignment = max(maxAlignment, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
		}
		if ((resourceUsage & RESOURCE_USAGE_ACCELERATION_STRUCTURE_BUFFER) != 0)
		{
			maxAlignment = max(maxAlignment, D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT);
		}
		if ((resourceUsage & RESOURCE_USAGE_SHADERTABLE_BUFFER) != 0)
		{
			maxAlignment = max(maxAlignment, D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT);

		}


		return maxAlignment;

	}


	void ResourceManagerDx12::prepare()
	{

		m_preFrameUploads->prepareNextUploadBatch();
		
		{
			std::unique_lock<std::mutex> lock(m_destroyObjectsMutex);
			m_destroyObjectsIndex = (m_destroyObjectsIndex + 1) % m_pendingDestroyedObjects.size();;
			m_pendingDestroyedObjects[m_destroyObjectsIndex].clear();
		}

		
	}

	void ResourceManagerDx12::uploadPreFrameData(FenceState* fencesToWait, size_t fenceCount, size_t queueIndex)
	{
		m_preFrameUploads->flushUploadBatch(fencesToWait, fenceCount, queueIndex);
		
		m_duringFrameUploads->prepareNextUploadBatch();

		m_frameNumber = m_duringFrameUploads->getFenceHelper().getCurrentFrameCount();

		if (m_frameNumber >= m_pipelineLength)
		{
			m_nonSamplerDescHeapAllocator->flushReleasedRanges(m_frameNumber - m_pipelineLength);
			m_samplerDescHeapAllocator->flushReleasedRanges(m_frameNumber - m_pipelineLength);;
		}

	}

	void ResourceManagerDx12::uploadFrameData(FenceState* fencesToWait, size_t fenceCount, size_t queueIndex)
	{
		m_duringFrameUploads->flushUploadBatch(fencesToWait, fenceCount, queueIndex);
	}

	void ResourceManagerDx12::issueWaitForLatestUploads(SubmissionThreadDx12& submissionThread, SubmissionThreadDx12::CommandQueueType queueType, size_t queueIndex)
	{
		ID3D12Fence* fence1 = m_preFrameUploads->getFenceHelper().getFenceForFrame(m_preFrameUploads->getFenceHelper().getCurrentFrameCount() - 1);
		ID3D12Fence* fence2 = m_duringFrameUploads->getFenceHelper().getFenceForFrame(m_duringFrameUploads->getFenceHelper().getCurrentFrameCount() - 1);

		FenceState fenceStates[] =
		{
			{ fence1, m_preFrameUploads->getFenceHelper().getCurrentFrameCount() - 1 },
			{ fence2, m_duringFrameUploads->getFenceHelper().getCurrentFrameCount() - 1 }
		};


		submissionThread.wait(queueType, queueIndex, fenceStates, 2);
		
	}

	ShaderModuleHandle ResourceManagerDx12::createShaderModule(const char* filepath, ShaderModuleType moduleType, const char* entryPoint, const ShaderModuleDefine* defines, size_t defineCount)
	{
		ShaderModuleHandle mod = new ShaderModuleDx12;

		std::wstring profileWStr;

		std::wstring filePathWStr;
		std::wstring entryPointWStr;

		shaderTypeToProfile(moduleType, profileWStr);
		stringToWString(filepath, filePathWStr);
		stringToWString(entryPoint, entryPointWStr);

		std::vector<DxcDefine> definesDxc;
		std::vector<std::wstring> defineWstr;
		size_t defineCountFinal = defineCount + 1;

		definesDxc.resize(defineCountFinal);
		defineWstr.resize(defineCount * 2);

		for (size_t i = 0; i < defineCount; ++i)
		{
			stringToWString(defines[i].name, defineWstr[i * 2]);

			definesDxc[i].Name = defineWstr[i * 2].c_str();
			if (defines[i].value != nullptr)
			{
				stringToWString(defines[i].value, defineWstr[i * 2 + 1]);
				definesDxc[i].Value = defineWstr[i * 2 + 1].c_str();
			}
		}

		definesDxc.back().Name = L"DX12";
		definesDxc.back().Value = L"1";
		

		mod->type = moduleType;
		mod->shaderBlob = compileFromFile(profileWStr.data(), entryPointWStr.data(), moduleType == ShaderModuleType::LIBRARY_MODULE ? L"" : entryPointWStr.data(), filePathWStr.data(), definesDxc.data(), (UINT32)defineCountFinal);
		if (mod->shaderBlob.get())
		{
			readReflectionData(mod->shaderBlob.get(), entryPoint, mod->reflection);
		}

		return mod;
	}
	void ResourceManagerDx12::destroyShaderModule(ShaderModuleHandle m)
	{
		delete m;
	}

}