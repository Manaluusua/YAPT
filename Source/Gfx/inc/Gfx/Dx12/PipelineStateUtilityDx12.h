#pragma once
#include <Gfx/Dx12/d3dx12.h>
#include <Gfx/Dx12/PipelineLayoutDx12.h>
namespace YAPT
{
	ID3D12RootSignature* createRootSignature(ID3D12Device5& device, const PipelineLayoutDx12* layout, D3D12_ROOT_SIGNATURE_FLAGS flags);
	ID3D12RootSignature* createRootSignature(ID3D12Device5& device, const D3D12_ROOT_PARAMETER* rootParams, size_t numberOfRootParams, const D3D12_STATIC_SAMPLER_DESC* staticSamplers, size_t numberOfStaticSamplers, D3D12_ROOT_SIGNATURE_FLAGS flags);
}