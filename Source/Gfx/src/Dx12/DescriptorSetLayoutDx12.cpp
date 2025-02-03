#include <Gfx/Dx12/DescriptorSetLayoutDx12.h>
#include <Gfx/Dx12/YaptToDx12Conversions.h>
#include <algorithm>

namespace YAPT
{
	///////////////////////////////////////////////////// DescriptorSetLayoutDx12 /////////////////////////////////////////////////////

	DescriptorSetLayoutDx12::DescriptorSetLayoutDx12(const DescriptorSetLayoutBinding* bindings, size_t numberOfBindings, DescriptorSetLayoutFlags flags)
		:m_flags(flags)
	{
		if (numberOfBindings > 0)
		{
			m_bindings.assign(bindings, bindings + numberOfBindings);
			std::sort(m_bindings.data(), m_bindings.data() + numberOfBindings, [](const DescriptorSetLayoutBinding& a, const DescriptorSetLayoutBinding& b)
				{
					return a.bindingIndex < b.bindingIndex;
				});

			m_perBindingData.resize(size_t(m_bindings.back().bindingIndex) + 1u, { size_t(-1) });
		}

		size_t dynamicDescriptors = 0;
		size_t nonDynamicDescriptors = 0;
		size_t dynamicSamplers = 0;

		for (size_t i = 0; i < m_bindings.size(); ++i)
		{
			DescriptorSetLayoutBinding& bindings = m_bindings[i];
			if (bindings.type == DescriptorType::UNIFORM_BUFFER_DYNAMIC || bindings.type == DescriptorType::STORAGE_BUFFER_DYNAMIC)
			{
				m_perBindingData[bindings.bindingIndex].offsetInDescTableOrRootParameter = dynamicDescriptors;
				dynamicDescriptors += bindings.descriptorCount;
			}
			else if (bindings.type == DescriptorType::SAMPLER)
			{
				if (bindings.staticSamplers == nullptr)
				{
					m_perBindingData[bindings.bindingIndex].offsetInDescTableOrRootParameter = dynamicSamplers;
					dynamicSamplers += bindings.descriptorCount;
				}
				
			} else
			{
				m_perBindingData[bindings.bindingIndex].offsetInDescTableOrRootParameter = nonDynamicDescriptors;
				nonDynamicDescriptors += bindings.descriptorCount;
			}

			m_perBindingData[bindings.bindingIndex].offsetToBindingDefinitions = i;
		}

		m_numberOfDynamicDescriptors = dynamicDescriptors;
		m_numberOfNonDynamicDescriptors = nonDynamicDescriptors;
		m_numberOfDynamicSamplerDescriptors = dynamicSamplers;
	}
	DescriptorSetLayoutDx12::~DescriptorSetLayoutDx12()
	{

	}
}