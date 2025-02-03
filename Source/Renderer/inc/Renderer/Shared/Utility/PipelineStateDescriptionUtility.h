#pragma once

#include <Gfx/GfxTypes.h>
#include <Renderer/Shared/Utility/ShaderLoader.h>

namespace YAPT
{
	
	bool areDefinitionsEqual(const VertexBufferDefinition& a, const VertexBufferDefinition& b);

	void fillDefaults(BlendStateDescription& desc);
	void fillDefaults(DepthStencilStateDescription& desc);
	void fillDefaults(RasterizerStateDescription& desc);
	void fillDefaults(MultisampleStateDescription& desc);
	void fillDefaults(ViewPort* viewPorts, ScissorRect* scissors, size_t count, float width, float height);
	void fillDefaults(BlendTargetDescription* descs, size_t count, bool enableBlend);

	void fillDefaults(SamplerDescription& desc, Filter min = Filter::NEAREST, Filter magn = Filter::NEAREST, Filter mip = Filter::NEAREST, SamplerAddressMode addressMode = SamplerAddressMode::REPEAT);

	void fillShaderModuleCreateInfo(const ShaderLoader::ShaderModuleInfo& from, ShaderStageCreateInfo& to);

	struct GraphicsPipelineStateDescHelper
	{
		GraphicsPipelineStateDescHelper& setDefaults(float width, float height, uint32_t blendTargetsCount = 1, bool blendEnable = false, uint32_t viewPortCount = 1);
		GraphicsPipelineStateDescHelper& setupShaderStages(const ShaderLoader::ShaderPipelineInfo* info);
		GraphicsPipelineStateDescHelper& setVertexBufferDefinitions(const VertexBufferDefinition* vertexBufferDefinitions, size_t numberOfDefinitions);


		GraphicsPipelineStateDesc pipelineDesc;

		std::vector<ShaderStageCreateInfo> shaderStages;
		std::vector<BlendTargetDescription> blendTargetDescriptions;
		std::vector<ViewPort> viewports;
		std::vector<ScissorRect> scissors;
		BlendStateDescription blendStateDescription;
		DepthStencilStateDescription depthStencilState;
		RasterizerStateDescription rasterizerStateDescription;
		MultisampleStateDescription multisampleState;
	};


	

}