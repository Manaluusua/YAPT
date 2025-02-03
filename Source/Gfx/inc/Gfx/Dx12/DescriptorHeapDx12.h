#ifndef YAPT_DX12_DESCRIPTORHEAP_H
#define YAPT_DX12_DESCRIPTORHEAP_H


#include <Gfx/Dx12/Dx12CommonIncludes.h>


namespace YAPT
{
	class ResourceManagerDx12;

	class DescriptorHeapDx12
	{

		friend class ResourceManagerDx12;

	public:
		size_t getDescriptorIncrementSize() const { return m_descriptorIncrementSize; }
		size_t getDescriptorCount() const { return m_numDescriptors; }

		D3D12_CPU_DESCRIPTOR_HANDLE getCPUDescriptorHandle(size_t index = 0) const;
		D3D12_GPU_DESCRIPTOR_HANDLE getGPUDescriptorHandle(size_t index = 0) const;

		void destroy();

		ID3D12DescriptorHeap* getNativeHeap() const { return m_descHeap.get(); };

	private:
		DescriptorHeapDx12(ResourceManagerDx12& resMngr, D3D12_DESCRIPTOR_HEAP_TYPE type, size_t descriptorCount, D3D12_DESCRIPTOR_HEAP_FLAGS flags);
		~DescriptorHeapDx12();

		ResourceManagerDx12& m_resMngr;
		RCPtr<ID3D12DescriptorHeap> m_descHeap;
		D3D12_DESCRIPTOR_HEAP_TYPE m_type;
		D3D12_DESCRIPTOR_HEAP_FLAGS m_flags;
		size_t m_descriptorIncrementSize;
		size_t m_numDescriptors;



	};
}
#endif