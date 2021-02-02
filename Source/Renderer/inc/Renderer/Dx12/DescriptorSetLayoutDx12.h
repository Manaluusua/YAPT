#pragma once

#include <Common/RCObject.h>
#include <Renderer/Shared/GfxTypes.h>

namespace YAPT
{
	class DescriptorSetLayoutDx12 : public RCObject
	{
		
	public:
		DescriptorSetLayoutDx12(const DescriptorSetLayoutBinding* bindings, size_t numberOfBindings, DescriptorSetLayoutFlags flags);
		~DescriptorSetLayoutDx12();

		size_t getNumberOfDynamicDescriptors() const { return m_numberOfDynamicDescriptors; }
		size_t getNumberOfNonDynamicDescriptors() const { return m_numberOfNonDynamicDescriptors; }
		size_t getNumberOfDynamicSamplerDescriptors() const { return m_numberOfDynamicSamplerDescriptors; }

		size_t getNumberOfDescriptorSetLayoutbindings() const{ return m_bindings.size(); }

		const DescriptorSetLayoutBinding& getDescriptorSetLayoutBinding(size_t index)
		{
			assert(index < m_bindings.size());
			return m_bindings[index];
		}
		 
		size_t getDescriptorTableOrRootParameterOffsetForBinding(size_t bindingIndex) const
		{
			assert(bindingIndex < m_perBindingData.size());
			return m_perBindingData[bindingIndex].offsetInDescTableOrRootParameter;
		}

		const DescriptorSetLayoutBinding& getDescriptorBindingDefinition(size_t bindingIndex) const
		{
			assert(bindingIndex < m_perBindingData.size());
			assert(m_perBindingData[bindingIndex].offsetToBindingDefinitions != size_t(-1));
			return m_bindings[m_perBindingData[bindingIndex].offsetToBindingDefinitions];
		} 
		
		bool hasBindingDefinitionForBindingIndex(size_t bindingIndex) const
		{
			if (bindingIndex >= m_perBindingData.size())
			{
				return false;
			}

			if (m_perBindingData[bindingIndex].offsetToBindingDefinitions == size_t(-1))
			{
				return false;
			}

			return true;
		}
		 
		DescriptorSetLayoutFlags getFlags() const { return m_flags; }

	private:

		struct PerBindingData
		{
			size_t offsetToBindingDefinitions;
			size_t offsetInDescTableOrRootParameter;
		};

		std::vector<DescriptorSetLayoutBinding> m_bindings;


		std::vector<PerBindingData> m_perBindingData;

		size_t m_numberOfDynamicDescriptors;
		size_t m_numberOfNonDynamicDescriptors;
		size_t m_numberOfDynamicSamplerDescriptors;
		DescriptorSetLayoutFlags m_flags;
	};
}