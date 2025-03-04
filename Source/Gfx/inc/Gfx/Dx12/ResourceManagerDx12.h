#ifndef YAPT_DX12_RESOURCEMANAGER_H
#define YAPT_DX12_RESOURCEMANAGER_H


#include <Gfx/Dx12/Dx12CommonIncludes.h>
#include <Gfx/Dx12/FenceHelperDx12.h>
#include <Gfx/GfxTypes.h>
#include <Gfx/Dx12/SubmissionThreadDx12.h>

#include <Gfx/GfxApi.h>
#include <Common/CommonUtilities.h>
#include <Common/GrowingMultiProducerPendingList.h>

#include <vector>


#define DEFAULT_UPLOAD_HEAP_SIZE 256 * 1e6
#define DEFAULT_DESCRIPTORHEAP_SIZE_NONSAMPLER 64000
#define DEFAULT_DESCRIPTORHEAP_SIZE_SAMPLER 1024

namespace D3D12MA
{
	class Allocator;
}

struct IDXGIAdapter;

namespace YAPT
{
	class DescriptorHeapDx12;
	class RendererDx12;
	class ResourceChunk;
	class BufferDx12;
	class TextureDx12;
	class UploadHelperDx12;
	class DescriptorHeapAllocatorDx12;

	class ResourceManagerDx12
	{
		friend class DescriptorHeapDx12;
		friend class RendererDx12;
	public:
		ResourceManagerDx12(ID3D12Device5& device, IDXGIAdapter& adapter, size_t pipelineLength);
		~ResourceManagerDx12();

		YAPT_NOCOPY(ResourceManagerDx12);

		bool initialize(SubmissionThreadDx12* submissionThread, size_t assetUploadHeapSize = DEFAULT_UPLOAD_HEAP_SIZE, size_t renderUploadHeapSize = DEFAULT_UPLOAD_HEAP_SIZE);

		DescriptorHeapDx12* createDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE type, size_t descriptorCount);
		
		Allocation* allocate(const D3D12_RESOURCE_DESC& resourceDesc, D3D12_HEAP_TYPE heapType, D3D12_RESOURCE_STATES initialState, const D3D12_CLEAR_VALUE* pOptimizedClearValue, REFIID riidResource, void** ppvResource);
		void deallocate(Allocation* a);

		ShaderModuleHandle createShaderModule(const char* filepath, ShaderModuleType moduleType, const char* entryPoint, const ShaderModuleDefine* defines, size_t defineCount);
		void destroyShaderModule(ShaderModuleHandle m);

		void upload(BufferHandleDx12* handle, size_t offsetInBytes, size_t sizeInBytes, const void* data, GpuUploadStage heapType);
		void* map(BufferHandleDx12* handle, size_t offsetInBytes, size_t sizeInBytes, GpuUploadStage heapType);
		void unmap(BufferHandleDx12* handle);
		void upload(TextureHandle image, size_t arraySliceOffset, size_t arraySliceCount, size_t mipOffset, size_t mipCount, const TextureDataDefinition* textureDataDefinitions, GpuUploadStage heapType);
		
		DescriptorSetPoolDx12* createDescriptorSetPool(const DescriptorSetLayoutDx12* layout, size_t numberOfDescriptorSets);
		void destroyDescriptorSetPool(DescriptorSetPoolDx12* pool);

		size_t getPipelineLength() const { return m_pipelineLength; };

		ID3D12Device5& getDevice() { return m_device; };

		void addToPendingDestructionList(Allocation* alloc)
		{
			addToPendingDestructionList(&alloc, 1);
		}
		void addToPendingDestructionList(ID3D12Object* objects)
		{
			addToPendingDestructionList(&objects, 1);
		}
		void addToPendingDestructionList(Allocation** allocs, size_t allocCount);
		void addToPendingDestructionList(ID3D12Object** objects, size_t objCount);

		size_t getFrameNumber() const;

		DescriptorHeapDx12* getNonSamplerDescHeap() const;
		DescriptorHeapDx12* getSamplerDescHeap() const;

		DescriptorHeapAllocatorDx12* getNonSamplerDescHeapAllocator() const { return m_nonSamplerDescHeapAllocator; }
		DescriptorHeapAllocatorDx12* getSamplerDescHeapAllocator() const { return m_samplerDescHeapAllocator; }

	private:

		void copyViaUploadHeap(ID3D12Resource* buffer, size_t offsetInBytes, size_t sizeInBytes, const void* data, GpuUploadStage heapType);
		void copyViaUploadHeap(ID3D12Resource* texture, const D3D12_RESOURCE_DESC& resourceDesc, size_t arraySliceOffset, size_t arraySliceCount,
			size_t mipOffset, size_t mipCount, const TextureDataDefinition* textureDataDefinitions, GpuUploadStage heapType);
		void* mapCopyRangeFromUploadHeap(ID3D12Resource* buffer, size_t offsetInBytes, size_t sizeInBytes, GpuUploadStage heapType);

		void prepare();
		void uploadPreFrameData(FenceState* fencesToWait, size_t fenceCount, size_t queueIndex = 0);
		void uploadFrameData(FenceState* fencesToWait, size_t fenceCount, size_t queueIndex = 0);

		void issueWaitForLatestUploads(SubmissionThreadDx12& submissionThread, SubmissionThreadDx12::CommandQueueType queueType, size_t queueIndex);

		void releaseDescriptorHeap(DescriptorHeapDx12* heap);

		void deinitialize();

		
		ID3D12Device5& m_device;
		IDXGIAdapter& m_adapter;
		RCPtr<D3D12MA::Allocator> m_memoryAllocator;

		UploadHelperDx12* m_preFrameUploads;
		UploadHelperDx12* m_duringFrameUploads;
		
		DescriptorHeapAllocatorDx12* m_nonSamplerDescHeapAllocator;
		DescriptorHeapAllocatorDx12* m_samplerDescHeapAllocator;

		SubmissionThreadDx12* m_submissionThread;

		//pending destruction lists
		std::vector<std::vector<RCPtr<ID3D12Object>>> m_pendingDestroyedObjects;
		std::vector<std::vector<Allocation*>> m_pendingFreedAllocations;
		std::mutex m_destroyObjectsMutex;
		std::mutex m_freeAllocationsMutex;
		size_t m_destroyPendingListIndex;

		size_t m_pipelineLength;
		size_t m_frameNumber;

	};
}
#endif