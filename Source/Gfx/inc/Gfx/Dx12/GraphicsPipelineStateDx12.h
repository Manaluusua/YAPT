#pragma once

#include <Gfx/GfxTypes.h>
#include <Gfx/Dx12/PipelineStateUtilityDx12.h>
namespace YAPT
{
	class ResourceManagerDx12;
	class GraphicsPipelineStateDx12
	{
	public:
		GraphicsPipelineStateDx12(ResourceManagerDx12& resMngr, const GraphicsPipelineStateDesc& desc);
		~GraphicsPipelineStateDx12();

		void bind(CommandBufferHandle cmdBuffer);
		size_t getStrideForVertexBuffer(size_t index) const { return m_perVertexBufferStrides[index]; }
	private:
		ResourceManagerDx12& m_resMngr;
		RCPtr<ID3D12RootSignature> m_signature;
		RCPtr<ID3D12PipelineState> m_pipelineState;
		RCPtr<PipelineLayoutDx12> m_layout;
		std::vector<size_t> m_perVertexBufferStrides;
		std::vector<D3D12_VIEWPORT> m_viewPorts;
		std::vector<D3D12_RECT> m_scissors;

		D3D12_PRIMITIVE_TOPOLOGY m_topology;
	};


}