#include <Renderer/Dx12/PipelineStateUtilityDx12.h>

namespace YAPT
{
	ID3D12RootSignature* createRootSignature(ID3D12Device5& device, const PipelineLayoutDx12* layout, D3D12_ROOT_SIGNATURE_FLAGS flags)
	{

		const D3D12_ROOT_PARAMETER* rootParams;
		size_t rootParamsCount = layout->getNumberOfRootParams();

		const D3D12_STATIC_SAMPLER_DESC* staticSamplers;
		size_t numberOfStaticSamplers;

		layout->getRootParameters(rootParams);
		layout->getStaticSamplers(staticSamplers, numberOfStaticSamplers);

		return createRootSignature(device, rootParams, rootParamsCount, staticSamplers, numberOfStaticSamplers, flags);
	}

	ID3D12RootSignature* createRootSignature(ID3D12Device5& device, const D3D12_ROOT_PARAMETER* rootParams, size_t numberOfRootParams, const D3D12_STATIC_SAMPLER_DESC* staticSamplers, size_t numberOfStaticSamplers, D3D12_ROOT_SIGNATURE_FLAGS flags)
	{
		CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;


		rootSignatureDesc.Init((UINT)numberOfRootParams, rootParams, (UINT)numberOfStaticSamplers, staticSamplers, flags);

		ID3D12RootSignature* signature;
		RCPtr<ID3DBlob> signatureBlob;
		RCPtr<ID3DBlob> error;
		HRESULT hres = D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &error);

		if (FAILED(hres))
		{
			YAPT_LOG_FATAL_ERROR_W_LENGTH(reinterpret_cast<char*>(error->GetBufferPointer()), error->GetBufferSize());
			return nullptr;
		}
		checkForDxError(device.CreateRootSignature(0, signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(), IID_PPV_ARGS(&signature)));
		return signature;
	}
}