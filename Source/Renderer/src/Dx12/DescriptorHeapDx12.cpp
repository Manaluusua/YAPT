#include <Renderer/Dx12/DescriptorHeapDx12.h>
#include <Renderer/Dx12/ResourceManagerDx12.h>
#include <Renderer/Dx12/d3dx12.h>
#include <cassert>
namespace YAPT
{

	DescriptorHeapDx12::DescriptorHeapDx12(ResourceManagerDx12& resMngr, D3D12_DESCRIPTOR_HEAP_TYPE type, size_t descriptorCount, D3D12_DESCRIPTOR_HEAP_FLAGS flags)
		:m_resMngr(resMngr)
	{
		m_numDescriptors = descriptorCount;
		m_descriptorIncrementSize = resMngr.getDevice().GetDescriptorHandleIncrementSize(type);
		m_type = type;
		m_flags = flags;

		D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
		rtvHeapDesc.NumDescriptors = (UINT)m_numDescriptors;
		rtvHeapDesc.Type = m_type;
		rtvHeapDesc.Flags = m_flags;
		if (FAILED(resMngr.getDevice().CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_descHeap))))
		{
			assert(false);
			m_numDescriptors = 0;
		}
		
		
	}

	D3D12_CPU_DESCRIPTOR_HANDLE DescriptorHeapDx12::getCPUDescriptorHandle(size_t index) const
	{
		CD3DX12_CPU_DESCRIPTOR_HANDLE handle(m_descHeap->GetCPUDescriptorHandleForHeapStart(), (UINT)index, (UINT)m_descriptorIncrementSize);
		return  handle;
	}
	D3D12_GPU_DESCRIPTOR_HANDLE DescriptorHeapDx12::getGPUDescriptorHandle(size_t index) const
	{
		CD3DX12_GPU_DESCRIPTOR_HANDLE handle(m_descHeap->GetGPUDescriptorHandleForHeapStart(), (UINT)index, (UINT)m_descriptorIncrementSize);
		return  handle;
	}

	void DescriptorHeapDx12::destroy()
	{
		m_resMngr.releaseDescriptorHeap(this);
	}

	DescriptorHeapDx12::~DescriptorHeapDx12()
	{

	}

}
