#include <Gfx/Dx12/ComputePipelineStateDx12.h>
#include <Gfx/Dx12/ResourceManagerDx12.h>
namespace YAPT
{
	ComputePipelineStateDx12::ComputePipelineStateDx12(ResourceManagerDx12& resMngr, const ComputePipelineStateDesc& desc)
		:m_resMngr(resMngr),
		m_layout(desc.pipelineLayout)
	{
		//assign without incrementing the counter
		*(&m_signature) = createRootSignature(resMngr.getDevice(), m_layout.get(), D3D12_ROOT_SIGNATURE_FLAG_DENY_VERTEX_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS | D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS | D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS);

		assert(m_signature.get() != nullptr);

		D3D12_COMPUTE_PIPELINE_STATE_DESC compPipelineStateDesc;
		compPipelineStateDesc.pRootSignature = m_signature.get();
		compPipelineStateDesc.CS.pShaderBytecode = desc.shaderStage.shaderModule->shaderBlob->GetBufferPointer();
		compPipelineStateDesc.CS.BytecodeLength = desc.shaderStage.shaderModule->shaderBlob->GetBufferSize();
		compPipelineStateDesc.NodeMask = 0;

		compPipelineStateDesc.CachedPSO.CachedBlobSizeInBytes = 0;
		compPipelineStateDesc.CachedPSO.pCachedBlob = nullptr;
		compPipelineStateDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;


		ID3D12PipelineState* state;
		checkForDxError(m_resMngr.getDevice().CreateComputePipelineState(&compPipelineStateDesc, IID_PPV_ARGS(&state)));
		*(&m_pipelineState) = state;

	}
	ComputePipelineStateDx12::~ComputePipelineStateDx12()
	{
	 
	}


	void ComputePipelineStateDx12::bind(CommandBufferHandle cmdBuffer)
	{
		cmdBuffer->cmdList->SetPipelineState(m_pipelineState.get());
		cmdBuffer->cmdList->SetComputeRootSignature(m_signature.get());
		cmdBuffer->boundLayout = m_layout.get();
		cmdBuffer->boundPSO = this;
		cmdBuffer->boundPsoType = CommandBufferHandleDx12::Compute;
	}
}