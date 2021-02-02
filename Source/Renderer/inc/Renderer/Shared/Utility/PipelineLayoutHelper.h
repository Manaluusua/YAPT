#pragma once
#include <Renderer/Shared/ShaderPipelineReflection.h>
#include <Renderer/Shared/GfxTypes.h>
#include <Renderer/Shared/Utility/DescriptorSetUtility.h>
#include <vector>
#include <unordered_map>
namespace YAPT
{
	class PipelineLayoutHelper
	{
	public:
		PipelineLayoutHelper();
		~PipelineLayoutHelper();

		void initFromShaderReflection(GfxApiHandle gfx, ShaderPipelineReflection* refl);
		void initRaytraceLayoutFromShaderReflections(GfxApiHandle gfx, ShaderPipelineReflection** reflections, size_t numberOfReflectionData); //relevant only for raytracing

		void setStaticSamplers(uint32_t descSetInd, uint32_t bindingDefinitionIndex, SamplerHandle* samplers);
		void setStaticSamplers(const ShaderPipelineReflection::NameMapping& mapping, SamplerHandle* samplers);

		void setExplicitDescriptorSetLayout(size_t descSetIndex, const DescriptorSetLayoutBinding* bindings, size_t bindingCount, DescriptorSetLayoutHandle layoutHandle);

		void setDynamic(size_t descSetIndex, size_t bindingPointIndex);

		void compile();
		
		PipelineLayoutHandle getPipelineLayoutHandle() const { return m_pipelineLayout; }
		DescriptorSetLayoutHandle getDescriptorSetLayoutHandle(size_t index) const { return index < m_descriptorSetLayouts.size() ? m_descriptorSetLayouts[index] : YAPT_NULL_HANDLE; }

		DescriptorSetUtility& getDescriptorSetUtility(size_t index) { return m_descriptorSetUtilities[index]; }

		DescriptorSetHandle allocateDescriptorSet(size_t index);
		void deallocateDescriptorSet(size_t index, DescriptorSetHandle handle);

		size_t getDescriptorSetCount() const { return m_descriptorSetLayouts.size(); }

	private:

		struct ExtraData
		{
			std::vector<SamplerHandle> samplerHandles;
		};

		struct DescriptorSetLayoutDef
		{
			std::vector<DescriptorSetLayoutBinding> bindings;
			std::vector<ExtraData> extraData;
		};

		std::vector<DescriptorSetLayoutDef> m_descriptorSetLayoutDefinitions;
		std::vector<DescriptorSetLayoutHandle> m_descriptorSetLayouts;
		std::vector<DescriptorSetUtility> m_descriptorSetUtilities;

		std::unordered_map<uint64_t, size_t> m_bindingPointToDescriptorSetLayoutDefIndex;

		PipelineLayoutHandle m_pipelineLayout;

		GfxApiHandle m_gfx;
	};
}