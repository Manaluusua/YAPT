#pragma once
#include <Gfx/GfxTypes.h>
#include <Gfx/Dx12/PipelineStateUtilityDx12.h>
namespace YAPT
{
	class ResourceManagerDx12;
	class ComputePipelineStateDx12
	{
	public:
		ComputePipelineStateDx12(ResourceManagerDx12& resMngr, const ComputePipelineStateDesc& desc);
		~ComputePipelineStateDx12();

		void bind(CommandBufferHandle cmdBuffer);

	private:
		ResourceManagerDx12& m_resMngr;
		RCPtr<ID3D12RootSignature> m_signature;
		RCPtr<ID3D12PipelineState> m_pipelineState;
		RCPtr<PipelineLayoutDx12> m_layout;
	};

}