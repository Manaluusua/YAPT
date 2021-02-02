#pragma once

#include <Renderer/Dx12/DescriptorHeapAllocatorDx12.h>
#include <Renderer/Shared/GfxTypes.h>
namespace YAPT
{
	class DescriptorSetLayoutDx12;
	class ResourceManagerDx12;
	class DescriptorSetPoolDx12;


	
	struct DescriptorSetDx12
	{
		enum class RootDescriptorType
		{
			CBV,
			SRV,
			UAV
		};

		struct RootDescriptor
		{
			D3D12_GPU_VIRTUAL_ADDRESS resourceAddress;
			RootDescriptorType type;
			bool consumesDynamicOffset;
		};

		DescriptorSetPoolDx12* pool;

		size_t idleSinceFrame;

		D3D12_CPU_DESCRIPTOR_HANDLE samplerHeapDescSetBaseCPU;
		D3D12_GPU_DESCRIPTOR_HANDLE samplerHeapDescSetBaseGPU;

		D3D12_CPU_DESCRIPTOR_HANDLE nonSamplerHeapDescSetBaseCPU;
		D3D12_GPU_DESCRIPTOR_HANDLE nonSamplerHeapDescSetBaseGPU;

		size_t samplerHeapIncrementSize;
		size_t nonSamplerHeapIncrementSize;

		std::vector<RootDescriptor> rootDescriptors;
	};

	class DescriptorSetPoolDx12
	{
	public:
		DescriptorSetPoolDx12(ResourceManagerDx12& resMngr, const DescriptorSetLayoutDx12* layout,DescriptorHeapAllocatorDx12* srvUavCbvHeap, DescriptorHeapAllocatorDx12* samplerHeap, size_t numberOfDescriptorSets);
		~DescriptorSetPoolDx12();

		DescriptorSetDx12* getDescriptorSet(size_t descSetIndex);
		void useDescriptorSet(DescriptorSetDx12* set);
		void freeDescriptorSet(DescriptorSetDx12* set);
		bool isDescriptorSetUnused(DescriptorSetDx12* handle);

		void updateDescriptorSet(DescriptorSetDx12* set, const DescriptorSetUpdate* updates, size_t updateCount);

	private:

		bool isBufferHandle(DescriptorType type);

		ResourceManagerDx12& m_resMngr;
		const DescriptorSetLayoutDx12* m_layout;
		DescriptorHeapAllocatorDx12* m_nonSamplerHeap;
		DescriptorHeapAllocatorDx12* m_samplerHeap;

		RangeAllocator::Range m_allocatedNonSamplerRange;
		RangeAllocator::Range m_allocatedSamplerRange;

		std::vector<DescriptorSetDx12> m_descSets;

	};
}