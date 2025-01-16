#include <Renderer/Dx12/DescriptorSetPoolDx12.h>
#include <Renderer/Dx12/DescriptorSetLayoutDx12.h>
#include <Renderer/Dx12/ResourceManagerDx12.h>
#include <Renderer/Dx12/DescriptorHeapDx12.h>
#include <Renderer/Dx12/YaptToDx12Conversions.h>
#include <Renderer/Dx12/AccelerationStructureBuilderDx12.h>



namespace YAPT
{
	DescriptorSetPoolDx12::DescriptorSetPoolDx12(ResourceManagerDx12& resMngr, const DescriptorSetLayoutDx12* layout, DescriptorHeapAllocatorDx12* srvUavCbvHeap, DescriptorHeapAllocatorDx12* samplerHeap, size_t numberOfDescriptorSets)
		:m_resMngr(resMngr),
		m_layout(layout),
		m_nonSamplerHeap(srvUavCbvHeap),
		m_samplerHeap(samplerHeap),
		m_allocatedNonSamplerRange{ 0,0 },
		m_allocatedSamplerRange{ 0,0 }
	{
		size_t nonSamplerDescTableSize = m_layout->getNumberOfNonDynamicDescriptors();
		size_t dynamicSamplerDescTableSize = m_layout->getNumberOfDynamicSamplerDescriptors();

		if (nonSamplerDescTableSize > 0)
		{
			m_allocatedNonSamplerRange = m_nonSamplerHeap->allocate(nonSamplerDescTableSize * numberOfDescriptorSets);
		}

		if (dynamicSamplerDescTableSize > 0)
		{
			m_allocatedSamplerRange = m_samplerHeap->allocate(dynamicSamplerDescTableSize * numberOfDescriptorSets);
		}


		m_descSets.resize(numberOfDescriptorSets);

		for (size_t i = 0; i < numberOfDescriptorSets; ++i)
		{
			DescriptorSetDx12& set = m_descSets[i];
			set.pool = this;
			set.idleSinceFrame = size_t(-1);

			if (nonSamplerDescTableSize > 0)
			{
				size_t offset = i * nonSamplerDescTableSize + m_allocatedNonSamplerRange.offset;
				const DescriptorHeapDx12* heap = m_nonSamplerHeap->getHeap();
				set.nonSamplerHeapDescSetBaseCPU = heap->getCPUDescriptorHandle(offset);
				set.nonSamplerHeapDescSetBaseGPU = heap->getGPUDescriptorHandle(offset);
				set.nonSamplerHeapIncrementSize = heap->getDescriptorIncrementSize();
			}
			else
			{
				set.nonSamplerHeapIncrementSize = 0;
			}

			if (dynamicSamplerDescTableSize > 0)
			{
				size_t offset = i * dynamicSamplerDescTableSize + m_allocatedSamplerRange.offset;
				const DescriptorHeapDx12* heap = m_samplerHeap->getHeap();
				set.samplerHeapDescSetBaseCPU = heap->getCPUDescriptorHandle(offset);
				set.samplerHeapDescSetBaseGPU = heap->getGPUDescriptorHandle(offset);
				set.samplerHeapIncrementSize = heap->getDescriptorIncrementSize();
			}
			else
			{
				set.samplerHeapIncrementSize = 0;
			}

			set.rootDescriptors.resize(m_layout->getNumberOfDynamicDescriptors());

		}

		
	}
	DescriptorSetPoolDx12::~DescriptorSetPoolDx12()
	{
		if (m_allocatedNonSamplerRange.numElements > 0)
		{
			m_nonSamplerHeap->release(m_allocatedNonSamplerRange, m_resMngr.getFrameNumber());
		}

		if (m_allocatedSamplerRange.numElements > 0)
		{
			m_nonSamplerHeap->release(m_allocatedNonSamplerRange, m_resMngr.getFrameNumber());
		}
	}


	DescriptorSetDx12* DescriptorSetPoolDx12::getDescriptorSet(size_t descSetIndex)
	{
		assert(descSetIndex < m_descSets.size());

		DescriptorSetDx12* set = &m_descSets[descSetIndex];
		return set;
	}

	void DescriptorSetPoolDx12::useDescriptorSet(DescriptorSetDx12* set)
	{
		set->idleSinceFrame = std::numeric_limits<size_t>::max() - 1;
	}

	void DescriptorSetPoolDx12::freeDescriptorSet(DescriptorSetDx12* set)
	{
		set->idleSinceFrame = m_resMngr.getFrameNumber();
	}
	bool DescriptorSetPoolDx12::isDescriptorSetUnused(DescriptorSetDx12* handle)
	{
		if (handle->idleSinceFrame == size_t(-1)) return true;

		return (handle->idleSinceFrame + m_resMngr.getPipelineLength()) <= m_resMngr.getFrameNumber();
	}

	bool DescriptorSetPoolDx12::isBufferHandle(DescriptorType type)
	{
		return !(type == DescriptorType::SAMPLER || type == DescriptorType::TEXTURE || type == DescriptorType::STORAGE_TEXTURE);
	}
	
	 
	void DescriptorSetPoolDx12::updateDescriptorSet(DescriptorSetDx12* set, const DescriptorSetUpdate* updates, size_t updateCount)
	{
		for (size_t updateIndex = 0; updateIndex < updateCount; ++updateIndex)
		{
			const DescriptorSetUpdate& update = updates[updateIndex];

			if (!m_layout->hasBindingDefinitionForBindingIndex(update.dstBinding))
			{
				//YAPT_LOG_WARNING("Tried to update descriptorset with binding index %d, but binding doesn't exist (maybe unused and optimized out?). Ignoring...", update.dstBinding)
				continue;
			}
			 
			size_t descriptorOffsetFromDescSetStart = m_layout->getDescriptorTableOrRootParameterOffsetForBinding(update.dstBinding);
			const DescriptorSetLayoutBinding& def = m_layout->getDescriptorBindingDefinition(update.dstBinding);

			if (def.type == DescriptorType::UNIFORM_BUFFER_DYNAMIC || def.type == DescriptorType::STORAGE_BUFFER_DYNAMIC)
			{
				for (size_t i = 0; i < update.descriptorCount; ++i)
				{
					
					const BufferViewHandle& buffHandle = update.descriptor.asBufferViewPtr()[i];
					DescriptorSetDx12::RootDescriptor& rootDesc = set->rootDescriptors[descriptorOffsetFromDescSetStart + update.dstArrayElement + i];
					rootDesc.resourceAddress = buffHandle->resource->GetGPUVirtualAddress() + buffHandle->desc.offsetInBytes;
					rootDesc.consumesDynamicOffset = true;
					if (def.type == DescriptorType::UNIFORM_BUFFER_DYNAMIC)
					{
						rootDesc.type = DescriptorSetDx12::RootDescriptorType::CBV;
					}
					else
					{
						if (def.accessFlags & AccessFlagsBits::ACCESS_FLAGS_WRITE)
						{
							rootDesc.type = DescriptorSetDx12::RootDescriptorType::UAV;
						}
						else
						{
							rootDesc.type = DescriptorSetDx12::RootDescriptorType::SRV;
						}
					}
					
				}
			}
			else
			{
				D3D12_DESCRIPTOR_RANGE_TYPE rangeType = yaptToDx12DescriptorRangeType(def);
				switch (rangeType)
				{
				case D3D12_DESCRIPTOR_RANGE_TYPE_SRV:
				{
					bool useBufferView = isBufferHandle(def.type);
					for (size_t i = 0; i < update.descriptorCount; ++i)
					{
						D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc;
						ID3D12Resource* res;
						if (useBufferView)
						{
							if (def.type == DescriptorType::ACCELERATION_STRUCTURE)
							{
								TopLevelAccelerationStructureHandle accStruct = update.descriptor.asAccelerationStructurePtr()[i];
								BufferViewDx12* bufferView = &accStruct->accelerationStructure;
								res = bufferView->resource;
								fillShaderResourceView(bufferView->desc, def, res, srvDesc);
								res = nullptr;
							}
							else
							{
								res = update.descriptor.asBufferViewPtr()[i]->resource.get();
								fillShaderResourceView(update.descriptor.asBufferViewPtr()[i]->desc, def, res, srvDesc);
							}
							
						}
						else
						{
							res = update.descriptor.asTextureViewPtr()[i]->resource.get();
							fillShaderResourceView(update.descriptor.asTextureViewPtr()[i]->desc, def, srvDesc);
						}
						
						CD3DX12_CPU_DESCRIPTOR_HANDLE heapAddress(set->nonSamplerHeapDescSetBaseCPU);
						heapAddress.Offset((INT)(descriptorOffsetFromDescSetStart + update.dstArrayElement + i), (UINT)set->nonSamplerHeapIncrementSize);

						m_resMngr.getDevice().CreateShaderResourceView(res, &srvDesc, heapAddress);
					}
					break;
				}
				case D3D12_DESCRIPTOR_RANGE_TYPE_UAV:
				{
					bool useBufferView = isBufferHandle(def.type);
					for (size_t i = 0; i < update.descriptorCount; ++i)
					{
						CD3DX12_CPU_DESCRIPTOR_HANDLE heapAddress(set->nonSamplerHeapDescSetBaseCPU);
						heapAddress.Offset((INT)(descriptorOffsetFromDescSetStart + update.dstArrayElement + i), (UINT)set->nonSamplerHeapIncrementSize);
						D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc;
						ID3D12Resource* res;
						if (useBufferView)
						{
							res = update.descriptor.asBufferViewPtr()[i]->resource.get();
							fillUnorderedAccessView(update.descriptor.asBufferViewPtr()[i]->desc, def, uavDesc);
						}
						else
						{
							res = update.descriptor.asTextureViewPtr()[i]->resource.get();
							fillUnorderedAccessView(update.descriptor.asTextureViewPtr()[i]->desc, def, uavDesc);
						}

						m_resMngr.getDevice().CreateUnorderedAccessView(res, nullptr, &uavDesc, heapAddress);
					}

				}
					
					break;
				case D3D12_DESCRIPTOR_RANGE_TYPE_CBV:
					for (size_t i = 0; i < update.descriptorCount; ++i)
					{
						CD3DX12_CPU_DESCRIPTOR_HANDLE heapAddress(set->nonSamplerHeapDescSetBaseCPU);
						heapAddress.Offset((INT)(descriptorOffsetFromDescSetStart + update.dstArrayElement + i), (UINT)set->nonSamplerHeapIncrementSize);
						D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc;
						fillConstantBufferView(update.descriptor.asBufferViewPtr()[i]->desc, def, update.descriptor.asBufferViewPtr()[i]->resource.get(), cbvDesc);
						m_resMngr.getDevice().CreateConstantBufferView(&cbvDesc, heapAddress);
					}
					break;
				case D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER:
					if (def.staticSamplers == nullptr)
					{
						for (size_t i = 0; i < update.descriptorCount; ++i)
						{
							const SamplerHandle& samplerHandle = update.descriptor.asSamplerPtr()[i];
							CD3DX12_CPU_DESCRIPTOR_HANDLE heapAddress(set->samplerHeapDescSetBaseCPU);
							heapAddress.Offset((INT)(descriptorOffsetFromDescSetStart + update.dstArrayElement + i), (UINT)set->samplerHeapIncrementSize);
							m_resMngr.getDevice().CreateSampler(&samplerHandle->samplerDesc, heapAddress);
						}
					}
					break;
				default:
					break;
				}
			}
		}
	}
}