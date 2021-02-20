#include <Renderer/Dx12/PipelineLayoutDx12.h>
#include <Renderer/Dx12/DescriptorSetPoolDx12.h>
#include <Renderer/Dx12/YaptToDx12Conversions.h>

#include <Renderer/Shared/GfxTypes.h>

namespace YAPT
{
	PipelineLayoutDx12::PipelineLayoutDx12(GfxApiHandle h, const DescriptorSetLayoutHandle* descSetLayout, size_t descSetLayoutCount)
		:m_renderer(h)
	{
		m_descSetLayouts.assign(descSetLayout, descSetLayout + descSetLayoutCount);
		createRootSignatureParameters();
	}
	PipelineLayoutDx12::~PipelineLayoutDx12()
	{

	}

	void PipelineLayoutDx12::getRootParameters(const D3D12_ROOT_PARAMETER*& rootParams) const
	{
		rootParams = m_rootParameters.data();
	}

	void PipelineLayoutDx12::getStaticSamplers(const D3D12_STATIC_SAMPLER_DESC*& samplerDescs, size_t& numberOfSamplers) const
	{
		samplerDescs = m_staticSamplers.data();
		numberOfSamplers = m_staticSamplers.size();
	}

	void PipelineLayoutDx12::createRootSignatureParameters()
	{

		size_t totalNumberOfRootDescriptors = 0;
		size_t totalNumberOfRootTables = 0;
		size_t totalNumberOfSamplerRootTables = 0;

		size_t numberOfTableRangesNonSampler = 0;
		size_t numberOfTableRangesSampler = 0;
		for (size_t i = 0; i < m_descSetLayouts.size(); ++i)
		{
			DescriptorSetLayoutDx12* descSetLayout = m_descSetLayouts[i].get();
			totalNumberOfRootDescriptors += descSetLayout->getNumberOfDynamicDescriptors();
			totalNumberOfRootTables += descSetLayout->getNumberOfNonDynamicDescriptors() > 0 ? 1 : 0;
			totalNumberOfSamplerRootTables += descSetLayout->getNumberOfDynamicSamplerDescriptors() > 0 ? 1 : 0;

			DescriptorSetLayoutFlags flags = descSetLayout->getFlags();

			bool hasAliasingBindingPoints = (descSetLayout->getFlags() & DESCRIPTORSETLAYOUTFLAG_BINDINGS_MAY_ALIAS) != 0;

			for (size_t k = 0; k < descSetLayout->getNumberOfDescriptorSetLayoutbindings(); ++k)
			{
				const DescriptorSetLayoutBinding& binding = descSetLayout->getDescriptorSetLayoutBinding(k);

				if (binding.type != DescriptorType::UNIFORM_BUFFER_DYNAMIC && binding.type != DescriptorType::STORAGE_BUFFER_DYNAMIC)
				{
					if (binding.type == DescriptorType::SAMPLER)
					{
						if (binding.staticSamplers == nullptr)
						{
							++totalNumberOfSamplerRootTables;
						}
						else
						{

							m_staticSamplers.resize(m_staticSamplers.size() + 1);
							D3D12_STATIC_SAMPLER_DESC& staticSamplerDesc = m_staticSamplers.back();

							for (size_t i = 0; i < binding.descriptorCount; ++i)
							{
								const D3D12_SAMPLER_DESC& sampDesc = binding.staticSamplers[i]->samplerDesc;
								staticSamplerDesc.Filter = sampDesc.Filter;
								staticSamplerDesc.AddressU = sampDesc.AddressU;
								staticSamplerDesc.AddressV = sampDesc.AddressV;
								staticSamplerDesc.AddressW = sampDesc.AddressW;
								staticSamplerDesc.MipLODBias = sampDesc.MipLODBias;
								staticSamplerDesc.MaxAnisotropy = sampDesc.MaxAnisotropy;
								staticSamplerDesc.ComparisonFunc = sampDesc.ComparisonFunc;
								staticSamplerDesc.BorderColor = dx12BorderColorToStaticBorderColor(sampDesc.BorderColor);
								staticSamplerDesc.MinLOD = sampDesc.MinLOD;
								staticSamplerDesc.MaxLOD = sampDesc.MaxLOD;
								staticSamplerDesc.ShaderRegister = (UINT)binding.bindingIndex;
								staticSamplerDesc.RegisterSpace = (UINT)i;
								staticSamplerDesc.ShaderVisibility = yaptToDx12ShaderVisibility(binding.shaderStages);
							}
						}
					}
					else
					{
						if (hasAliasingBindingPoints)
						{
							numberOfTableRangesNonSampler += MAX_NUMBER_OF_ALLOWED_BINDINGPOINT_ALIASES;
						}
						else
						{
							++numberOfTableRangesNonSampler;
						}
						
					}

				}
			}

		}
		m_rootParameters.resize(totalNumberOfRootDescriptors + totalNumberOfRootTables + totalNumberOfSamplerRootTables);
		m_descriptorTableRangesNonSampler.resize(numberOfTableRangesNonSampler);
		m_descriptorTableRangesSampler.resize(numberOfTableRangesSampler);
		m_rootParameterToDescSetMapping.resize(m_rootParameters.size());
		m_descSetToRootParametersMapping.resize(m_descSetLayouts.size());

		{
			size_t currentDescriptorTableOffsetNonSampler = 0;
			size_t currentDescriptorTableOffsetSampler = 0;
			size_t currentRootDescriptorOffset = 0;

			size_t currentTableRangeOffsetNonSampler = 0;
			size_t currentTableRangeOffsetSampler = 0;
			 
			for (size_t i = 0; i < m_descSetLayouts.size(); ++i)
			{
				m_descSetToRootParametersMapping[i].rootDescriptorsRootParameterOffset = currentRootDescriptorOffset;
				m_descSetToRootParametersMapping[i].descriptorTableRootParameterOffsetNonSampler = totalNumberOfRootDescriptors + currentDescriptorTableOffsetNonSampler;
				m_descSetToRootParametersMapping[i].descriptorTableRootParameterOffsetSampler = totalNumberOfRootDescriptors + totalNumberOfRootTables + currentDescriptorTableOffsetSampler;

				DescriptorSetLayoutDx12* descSetLayout = m_descSetLayouts[i].get();

				bool hasAliasingBindingPoints = (descSetLayout->getFlags() & DESCRIPTORSETLAYOUTFLAG_BINDINGS_MAY_ALIAS) != 0;

				size_t tableRangeOffsetForThisDescSetNonSampler = currentTableRangeOffsetNonSampler;
				size_t tableRangeOffsetForThisDescSetSampler = currentTableRangeOffsetSampler;

				size_t currentDynamicDescriptorOffsetPerSet = 0;

				UINT currentNonSamplerDescriptorCountFromTableStart = 0;
				UINT currentSamplerDescriptorCountFromTableStart = 0;

				for (size_t k = 0; k < descSetLayout->getNumberOfDescriptorSetLayoutbindings(); ++k)
				{
					const DescriptorSetLayoutBinding& binding = descSetLayout->getDescriptorSetLayoutBinding(k);

					if (binding.type == DescriptorType::UNIFORM_BUFFER_DYNAMIC || binding.type == DescriptorType::STORAGE_BUFFER_DYNAMIC)
					{

						D3D12_ROOT_PARAMETER& rootParam = m_rootParameters[currentRootDescriptorOffset];

						if (binding.type == DescriptorType::UNIFORM_BUFFER_DYNAMIC)
						{
							rootParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
						}
						else
						{
							if ((binding.accessFlags & ACCESS_FLAGS_WRITE) == 0)
							{
								rootParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
							}
							else
							{
								rootParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_UAV;
							}
						}

						rootParam.Descriptor.RegisterSpace = (UINT)i;
						rootParam.Descriptor.ShaderRegister = binding.bindingIndex;
						rootParam.ShaderVisibility = yaptToDx12ShaderVisibility(binding.shaderStages);

						m_rootParameterToDescSetMapping[currentRootDescriptorOffset].isRootParam = true;
						m_rootParameterToDescSetMapping[currentRootDescriptorOffset].descriptorSetIndex = i;
						m_rootParameterToDescSetMapping[currentRootDescriptorOffset].rootParameterOffset = currentDynamicDescriptorOffsetPerSet;

						++currentDynamicDescriptorOffsetPerSet;

						++currentRootDescriptorOffset;
					} 
					else
					{
						if (binding.type == DescriptorType::SAMPLER)
						{
							if (binding.staticSamplers == nullptr)
							{
								D3D12_DESCRIPTOR_RANGE& descRange = m_descriptorTableRangesSampler[currentTableRangeOffsetSampler];
								descRange.RangeType = yaptToDx12DescriptorRangeType(binding);
								descRange.NumDescriptors = binding.descriptorCount;
								descRange.BaseShaderRegister = binding.bindingIndex;
								descRange.RegisterSpace = (UINT)i;
								descRange.OffsetInDescriptorsFromTableStart = currentSamplerDescriptorCountFromTableStart;

								currentSamplerDescriptorCountFromTableStart += binding.descriptorCount;
								++currentTableRangeOffsetSampler;
							}
						} 
						else
						{
							size_t numberOfaliases = hasAliasingBindingPoints ? MAX_NUMBER_OF_ALLOWED_BINDINGPOINT_ALIASES : 1;
							for (size_t aliasIndex = 0; aliasIndex < numberOfaliases; ++aliasIndex)
							{
								D3D12_DESCRIPTOR_RANGE& descRange = m_descriptorTableRangesNonSampler[currentTableRangeOffsetNonSampler];
								descRange.RangeType = yaptToDx12DescriptorRangeType(binding);
								descRange.NumDescriptors = binding.descriptorCount;
								descRange.BaseShaderRegister = binding.bindingIndex;
								descRange.RegisterSpace = (UINT)(i + aliasIndex * BINDINGPOINT_ALIAS_MULTIPLE);
								descRange.OffsetInDescriptorsFromTableStart = currentNonSamplerDescriptorCountFromTableStart;

								++currentTableRangeOffsetNonSampler;
							}

							currentNonSamplerDescriptorCountFromTableStart += binding.descriptorCount;
							
						}
					}
					
				}
				 
				if (descSetLayout->getNumberOfNonDynamicDescriptors() > 0)
				{
					size_t rootParamIndex = totalNumberOfRootDescriptors + currentDescriptorTableOffsetNonSampler;
					D3D12_ROOT_PARAMETER& rootParam = m_rootParameters[rootParamIndex];
					rootParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
					rootParam.DescriptorTable.NumDescriptorRanges = (UINT)(currentTableRangeOffsetNonSampler - tableRangeOffsetForThisDescSetNonSampler);
					rootParam.DescriptorTable.pDescriptorRanges = &m_descriptorTableRangesNonSampler[tableRangeOffsetForThisDescSetNonSampler];
					rootParam.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

					m_rootParameterToDescSetMapping[rootParamIndex].isRootParam = false;
					m_rootParameterToDescSetMapping[rootParamIndex].descriptorSetIndex = i;
					m_rootParameterToDescSetMapping[rootParamIndex].rootParameterOffset = 0;

					++currentDescriptorTableOffsetNonSampler;
				}

				if (descSetLayout->getNumberOfDynamicSamplerDescriptors() > 0)
				{
					size_t rootParamIndex = totalNumberOfRootDescriptors + totalNumberOfRootTables + currentDescriptorTableOffsetSampler;

					D3D12_ROOT_PARAMETER& rootParam = m_rootParameters[rootParamIndex];
					rootParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
					rootParam.DescriptorTable.NumDescriptorRanges = (UINT)(currentTableRangeOffsetSampler - tableRangeOffsetForThisDescSetSampler);
					rootParam.DescriptorTable.pDescriptorRanges = &m_descriptorTableRangesSampler[tableRangeOffsetForThisDescSetSampler];
					rootParam.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

					m_rootParameterToDescSetMapping[rootParamIndex].isRootParam = false;
					m_rootParameterToDescSetMapping[rootParamIndex].descriptorSetIndex = i;
					m_rootParameterToDescSetMapping[rootParamIndex].rootParameterOffset = 0;

					++currentDescriptorTableOffsetSampler;
				}

			}
		}

	}


	void PipelineLayoutDx12::bindRootParameters(GraphicsCommandListDx12* cmdList, const DescriptorSetBinding* bindings, size_t numberOfBindings, size_t* dynamicOffsets, size_t dynamicOffsetCount, bool isCompute) const
	{
		size_t currentDynamicOffsetIndex = 0;
		for (size_t i = 0; i < numberOfBindings; ++i)
		{
			DescriptorSetDx12* descSet = bindings[i].descSet;
			const DescriptorSetToRootParametersMapping& mappings = m_descSetToRootParametersMapping[bindings[i].descSetIndex];


			for (size_t rootDescInd = 0; rootDescInd < descSet->rootDescriptors.size(); ++rootDescInd)
			{
				UINT rootParameterIndex = (UINT)(mappings.rootDescriptorsRootParameterOffset + rootDescInd);
				const DescriptorSetDx12::RootDescriptor& rootDescInfo = descSet->rootDescriptors[rootDescInd];

				assert(currentDynamicOffsetIndex < dynamicOffsetCount);

				UINT offset = rootDescInfo.consumesDynamicOffset ? (UINT)dynamicOffsets[currentDynamicOffsetIndex] : 0;
				if (rootDescInfo.consumesDynamicOffset)
				{
					++currentDynamicOffsetIndex;
				}


				if (isCompute)
				{
					switch (rootDescInfo.type)
					{
					case YAPT::DescriptorSetDx12::RootDescriptorType::CBV:
						cmdList->SetComputeRootConstantBufferView(rootParameterIndex, rootDescInfo.resourceAddress + offset);
						break;
					case YAPT::DescriptorSetDx12::RootDescriptorType::SRV:
						cmdList->SetComputeRootShaderResourceView(rootParameterIndex, rootDescInfo.resourceAddress + offset);
						break;
					case YAPT::DescriptorSetDx12::RootDescriptorType::UAV:
						cmdList->SetComputeRootUnorderedAccessView(rootParameterIndex, rootDescInfo.resourceAddress + offset);
						break;
					default:
						YAPT_LOG_FATAL_ERROR("Should never reach here");
					}
				}
				else
				{
					switch (rootDescInfo.type)
					{
					case YAPT::DescriptorSetDx12::RootDescriptorType::CBV:
						cmdList->SetGraphicsRootConstantBufferView(rootParameterIndex, rootDescInfo.resourceAddress + offset);
						break;
					case YAPT::DescriptorSetDx12::RootDescriptorType::SRV:
						cmdList->SetGraphicsRootShaderResourceView(rootParameterIndex, rootDescInfo.resourceAddress + offset);
						break;
					case YAPT::DescriptorSetDx12::RootDescriptorType::UAV:
						cmdList->SetGraphicsRootUnorderedAccessView(rootParameterIndex, rootDescInfo.resourceAddress + offset);
						break;
					default:
						YAPT_LOG_FATAL_ERROR("Should never reach here");
					}
				}

			}

			if (descSet->nonSamplerHeapIncrementSize > 0)
			{
				UINT rootParameterIndex = (UINT)mappings.descriptorTableRootParameterOffsetNonSampler;
				if (isCompute)
				{
					cmdList->SetComputeRootDescriptorTable(rootParameterIndex, descSet->nonSamplerHeapDescSetBaseGPU);
				}
				else
				{
					cmdList->SetGraphicsRootDescriptorTable(rootParameterIndex, descSet->nonSamplerHeapDescSetBaseGPU);
				}

			}

			if (descSet->samplerHeapIncrementSize > 0)
			{
				UINT rootParameterIndex = (UINT)mappings.descriptorTableRootParameterOffsetSampler;
				if (isCompute)
				{
					cmdList->SetComputeRootDescriptorTable(rootParameterIndex, descSet->samplerHeapDescSetBaseGPU);
				}
				else
				{
					cmdList->SetGraphicsRootDescriptorTable(rootParameterIndex, descSet->samplerHeapDescSetBaseGPU);
				}

			}


		}
	}
}