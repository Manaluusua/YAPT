#include <Renderer/Shared/Utility/PipelineStateDescriptionUtility.h>
#include <Gfx/GfxBasicTypesUtility.h>
#include <limits>
namespace YAPT
{
	
	bool areDefinitionsEqual(const VertexBufferDefinition& a, const VertexBufferDefinition& b)
	{
		if (a.numberOfVertexAttributes != b.numberOfVertexAttributes) return false;
		if (a.stride != b.stride) return false;
		if (a.vertexInputRate != b.vertexInputRate) return false;

		for (size_t i = 0; i < b.numberOfVertexAttributes; ++i)
		{
			const VertexInputAttribute& attribA = a.attributes[i];
			const VertexInputAttribute& attribB = b.attributes[i];
			if ((attribA.format != attribB.format) ||
				(attribA.perVertexOffset != attribB.perVertexOffset) || 
				(attribA.shaderInputSlot.getIndex() != attribB.shaderInputSlot.getIndex()) ||
				(attribA.shaderInputSlot.getType() != attribB.shaderInputSlot.getType()))
			{
				return false;
			}
		}

		return true;
	}

	

	void fillDefaults(BlendTargetDescription* descs, size_t count, bool blendEnable)
	{
		for (size_t i = 0; i < count; ++i)
		{
			descs[i].blendEnable = blendEnable;
			descs[i].colorBlendOp = BlendOp::ADD;
			descs[i].srcColor = BlendFactor::SRC_ALPHA;
			descs[i].dstColor = BlendFactor::ONE_MINUS_SRC_ALPHA;

			descs[i].alphaBlendOp = BlendOp::ADD;
			descs[i].srcAlpha = BlendFactor::ONE;
			descs[i].dstAlpha = BlendFactor::ONE;

			descs[i].colorWriteMask = ColorMaskBits::ALL;
		}
		

	}

	void fillDefaults(BlendStateDescription& desc)
	{
		desc.enableIndependentBlend = false;
		desc.blendTargetDescriptions = nullptr;
		desc.numberOfBlendTargets = 0;
		desc.enableLogicalOp = false;
		desc.logicalOp = LogicOp::CLEAR;
		desc.blendConstants[0] = desc.blendConstants[1] = desc.blendConstants[2] = desc.blendConstants[3] = 0.f;
	}
	void fillDefaults(DepthStencilStateDescription& desc)
	{
		desc.depthCompareOp = CompareOp::LESS_OR_EQUAL;
		
		desc.stencilFront.failOp = StencilOp::KEEP;
		desc.stencilFront.passOp = StencilOp::KEEP;
		desc.stencilFront.depthFailOp = StencilOp::KEEP;
		desc.stencilFront.compareOp = CompareOp::ALWAYS;
		desc.stencilFront.reference = 0;

		desc.stencilBack.failOp = StencilOp::KEEP;
		desc.stencilBack.passOp = StencilOp::KEEP;
		desc.stencilBack.depthFailOp = StencilOp::KEEP;
		desc.stencilBack.compareOp = CompareOp::ALWAYS;
		desc.stencilBack.reference = 0;

		desc.compareMask = 0xFFFFFFFF;
		desc.writeMask = 0xFFFFFFFF;

		desc.minDepthBounds = 0.0f;
		desc.maxDepthBounds = 1.0f;
		desc.depthTestEnable = false;
		desc.depthWriteEnable = false;
		desc.depthBoundsTestEnable = false;
		desc.stencilTestEnable = false;
	}
	void fillDefaults(RasterizerStateDescription& desc)
	{
		desc.polygonMode = PolygonMode::FILL;
		desc.cullMode = CullMode::BACK;
		desc.frontFace = FrontFace::COUNTER_CLOCKWISE;
		desc.depthBiasEnable = false;
		desc.depthBiasConstantFactor = 0.f;
		desc.depthBiasClamp = 0.f;
		desc.depthBiasSlopeFactor = 0.f;
		desc.lineWidth = 1.f;
		desc.rasterizerDiscardEnable = false;
		desc.depthClampEnable = false;
	}
	void fillDefaults(MultisampleStateDescription& desc)
	{
		desc.sampleMask = 0x1;
		desc.sampleCount = SampleCount::SAMPLE_COUNT_1;
		desc.minSampleShading = 0.0f;
		desc.enableSampleShading = false;
		desc.enableAlphaToCoverage = false;
		desc.enableAlphaToOne = false;

	}
	void fillDefaults(ViewPort* viewPorts, ScissorRect* scissors, size_t count, float width, float height)
	{
		for (size_t i = 0; i < count; ++i)
		{
			ViewPort& vp = viewPorts[i];
			ScissorRect& sc = scissors[i];

			vp.x = 0.f;
			vp.y = 0.f;
			vp.width = width;
			vp.height = height;
			vp.minDepth = 0.f;
			vp.maxDepth = 1.f;

			sc.x = 0;
			sc.y = 0;
			sc.width = (uint32_t)width;
			sc.height = (uint32_t)height;
		}
	}

	void fillDefaults(SamplerDescription& desc, Filter min, Filter magn, Filter mip, SamplerAddressMode addressMode)
	{
		desc.minFilter = min;
		desc.magFilter = magn;
		desc.mipmapMode = mip;
		desc.addressModeU = desc.addressModeV = desc.addressModeW = addressMode;

		desc.borderColor = BorderColor::FLOAT_TRANSPARENT_BLACK;
		desc.compareOp = CompareOp::ALWAYS;
		desc.enableAnisotropy = false;
		desc.enableCompare = false;
		desc.maxAnisotropy = 1.0f;
		desc.minLod = 0;
		desc.maxLod = std::numeric_limits<float>().max();
		desc.mipLodBias = 0;
	}

	void fillShaderModuleCreateInfo(const ShaderLoader::ShaderModuleInfo& from, ShaderStageCreateInfo& to)
	{
		to.entryPoint = from.entryPoint;
		to.shaderModule = from.handle;
		to.stage = shaderModuleTypeToShaderStageBit(from.moduletype);
	}


	GraphicsPipelineStateDescHelper& GraphicsPipelineStateDescHelper::setupShaderStages(const ShaderLoader::ShaderPipelineInfo* info)
	{
		shaderStages.resize(info->shaderModules.size());
		for (size_t i = 0; i < info->shaderModules.size(); ++i)
		{
			fillShaderModuleCreateInfo(info->shaderModules[i], shaderStages[i]);
		}

		pipelineDesc.shaderStages = shaderStages.data();
		pipelineDesc.numberOfShaderStages = (uint32_t)shaderStages.size();

		return *this;
	}
	GraphicsPipelineStateDescHelper& GraphicsPipelineStateDescHelper::setVertexBufferDefinitions(const VertexBufferDefinition* vertexBufferDefinitions, size_t numberOfDefinitions)
	{
		pipelineDesc.vertexBufferLayouts = vertexBufferDefinitions;
		pipelineDesc.numberOfVertexBufferLayouts = (uint32_t)numberOfDefinitions;

		return *this;
	}

	GraphicsPipelineStateDescHelper& GraphicsPipelineStateDescHelper::setDefaults(float width, float height, uint32_t blendTargetsCount, bool blendEnable, uint32_t viewPortCount)
	{
		blendTargetDescriptions.resize(blendTargetsCount);
		viewports.resize(viewPortCount);
		scissors.resize(viewPortCount);

		fillDefaults(viewports.data(), scissors.data(), viewPortCount, width, height);
		fillDefaults(blendTargetDescriptions.data(), blendTargetsCount, blendEnable);

		fillDefaults(blendStateDescription);
		fillDefaults(depthStencilState);
		fillDefaults(rasterizerStateDescription);
		fillDefaults(multisampleState);

		blendStateDescription.blendTargetDescriptions = blendTargetDescriptions.data();
		blendStateDescription.numberOfBlendTargets = (uint32_t)blendTargetDescriptions.size();

		pipelineDesc.blendStateDescription = &blendStateDescription;
		pipelineDesc.depthStencilState = &depthStencilState;
		pipelineDesc.rasterizerStateDescription = &rasterizerStateDescription;
		pipelineDesc.multisampleState = &multisampleState;

		pipelineDesc.viewports = viewports.data();
		pipelineDesc.scissors = scissors.data();
		pipelineDesc.numberOfViewportsAndScissors = viewPortCount;

		pipelineDesc.primitivetopology = PrimitiveTopology::TRIANGLELIST;
		pipelineDesc.tesselationPatchControlPointCount = 0;
		pipelineDesc.dynamicPipelineStates = DYNAMIC_PIPELINESTATE_NONE;
		
		return *this;
	}
}