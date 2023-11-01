#pragma once

#include "DescriptorSetLayoutDx12.h"
#include <Common/RCObjectPtr.h>

namespace YAPT
{
	class PipelineLayoutDx12 : public RCObject
	{
	public:
		struct DescriptorSetToRootParametersMapping
		{
			size_t rootDescriptorsRootParameterOffset;
			size_t descriptorTableRootParameterOffsetNonSampler;
			size_t descriptorTableRootParameterOffsetSampler;
		};


		PipelineLayoutDx12(GfxApiHandle h, const DescriptorSetLayoutHandle*, size_t descSetLayoutCount);
		~PipelineLayoutDx12();

		void getRootParameters(const D3D12_ROOT_PARAMETER*& rootParams) const;
		void getStaticSamplers(const D3D12_STATIC_SAMPLER_DESC*& samplerDescs, size_t& numberOfSamplers) const;

		void bindRootParameters(GraphicsCommandListDx12* cmdList, const DescriptorSetHandle* binding, size_t bindingOffset, size_t numberOfBindings, size_t* dynamicOffsets, size_t dynamicOffsetCount, bool isCompute) const;

		size_t getNumberOfRootParams() const { return m_rootParameters.size(); };

		const DescriptorSetToRootParametersMapping& getDescSetToRootParametersMapping(size_t descSetIndex) const { return m_descSetToRootParametersMapping[descSetIndex]; }

	private:
		struct RootParameterToDescriptorSetMapping
		{
			size_t descriptorSetIndex;
			size_t rootParameterOffset;
			bool isRootParam;
		};

		
		void createRootSignatureParameters();
		GfxApiHandle m_renderer;
		std::vector<RCObjectPtr<DescriptorSetLayoutDx12>> m_descSetLayouts;
		std::vector<RootParameterToDescriptorSetMapping> m_rootParameterToDescSetMapping;
		std::vector<DescriptorSetToRootParametersMapping> m_descSetToRootParametersMapping;
		std::vector<D3D12_ROOT_PARAMETER> m_rootParameters;
		std::vector<D3D12_DESCRIPTOR_RANGE> m_descriptorTableRangesNonSampler;
		std::vector<D3D12_DESCRIPTOR_RANGE> m_descriptorTableRangesSampler;

		std::vector< D3D12_STATIC_SAMPLER_DESC> m_staticSamplers;
	};
}