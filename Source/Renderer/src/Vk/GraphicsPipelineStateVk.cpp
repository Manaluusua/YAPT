#include <Renderer/Vk/GraphicsPipelineStateVk.h>
#include <Renderer/Vk/RendererVk.h>
#include <Renderer/Vk/ResourceManagerVk.h>
#include <Renderer/Vk/ShaderModuleVk.h>
#include <Renderer/Vk/YaptToVkConversions.h>

namespace YAPT
{
	uint32_t getInputLocationForSemantic(AttributeSemantic semantic, const std::vector<ShaderModuleVk::InputAttributes>& inputAttributes)
	{
		for each (ShaderModuleVk::InputAttributes attr in inputAttributes)
		{
			if (semantic == attr.semantic)
			{
				return attr.location;
			}
		}
		return -1;
	}

	VkPipeline GraphicsPipelineStateVk::create(RendererVk* renderer, const GraphicsPipelineStateDesc& desc)
	{

		VkPipeline pipeline = VK_NULL_HANDLE;
		
		VkGraphicsPipelineCreateInfo createInfo{};
		createInfo.pNext = nullptr;
		createInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		createInfo.flags = 0;
		createInfo.basePipelineHandle = VK_NULL_HANDLE;
		createInfo.basePipelineIndex = -1;

		createInfo.layout = desc.pipelineLayout;
		createInfo.renderPass = desc.renderPass->pass;
		createInfo.subpass = desc.renderPass->index;
		
		//shader stages
		std::vector<VkPipelineShaderStageCreateInfo> stageCreateInfos;
		stageCreateInfos.resize(desc.numberOfShaderStages);
		for (size_t i = 0; i < stageCreateInfos.size(); ++i)
		{
			const ShaderStageCreateInfo& srcInfo = desc.shaderStages[i];
			VkPipelineShaderStageCreateInfo& dstInfo = stageCreateInfos[i];
			dstInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			dstInfo.pNext = NULL;
			dstInfo.flags = 0;
			dstInfo.module = srcInfo.shaderModule->m_vkShaderModule;
			dstInfo.pName = srcInfo.entryPoint;
			dstInfo.stage = yaptShaderStageBitstoVk(srcInfo.stage);
			dstInfo.pSpecializationInfo = NULL;
		}
		createInfo.stageCount = desc.numberOfShaderStages;
		createInfo.pStages = stageCreateInfos.data();

		VkPipelineDepthStencilStateCreateInfo dsvInfo;
		VkPipelineColorBlendStateCreateInfo colorBlendInfo;
		VkPipelineRasterizationStateCreateInfo rasterInfo;
		VkPipelineMultisampleStateCreateInfo multiSampleInfo;
		VkSampleMask sampleMask[2];
		std::vector<VkPipelineColorBlendAttachmentState> blendStates;
		blendStates.resize(desc.blendStateDescription->numberOfBlendTargets);
		{
			yaptDepthStencilStateToVk(*desc.depthStencilState, dsvInfo);
			yaptColorBlendStateToVk(*desc.blendStateDescription, colorBlendInfo, blendStates.data());
			yaptRasterizerStateToVk(*desc.rasterizerStateDescription, rasterInfo);
			yaptMultisampleStateToVk(*desc.multisampleState, multiSampleInfo, sampleMask);
		}

		colorBlendInfo.pAttachments = blendStates.data();

		//Vertex input
		VkPipelineVertexInputStateCreateInfo vertexInputInfo;
		std::vector<VkVertexInputAttributeDescription> attributeDescs;
		std::vector<VkVertexInputBindingDescription> bindingDescs;
		{
			size_t numberOfAttributeDescs = 0;
			for (size_t i = 0; i < desc.numberOfVertexBufferLayouts; ++i)
			{
				numberOfAttributeDescs += desc.vertexBufferLayouts[i].numberOfVertexAttributes;
			}

			vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
			vertexInputInfo.pNext = nullptr;
			vertexInputInfo.flags = 0;

			bindingDescs.resize(desc.numberOfVertexBufferLayouts);
			attributeDescs.resize(numberOfAttributeDescs);

			size_t attributeDescOffset = 0;
			ShaderModuleHandle vertexShader = nullptr;
			for (size_t i = 0; i < desc.numberOfShaderStages; ++i)
			{
				if (desc.shaderStages[i].stage == SHADERSTAGE_VERTEX)
				{
					vertexShader = desc.shaderStages[i].shaderModule;
					break;
				}
			}

			assert(vertexShader != nullptr);

			for (size_t i = 0; i < desc.numberOfVertexBufferLayouts; ++i)
			{
				const VertexBufferDefinition& srcDef = desc.vertexBufferLayouts[i];
				VkVertexInputBindingDescription& dstDef = bindingDescs[i];

				dstDef.stride = srcDef.stride;
				dstDef.inputRate = srcDef.vertexInputRate == 0 ? VK_VERTEX_INPUT_RATE_VERTEX : VK_VERTEX_INPUT_RATE_INSTANCE;
				dstDef.binding = (uint32_t)i;

				for (size_t k = 0; k < srcDef.numberOfVertexAttributes; ++k)
				{
					VkVertexInputAttributeDescription& attribDesc = attributeDescs[attributeDescOffset++];
					const VertexInputAttribute& attributeSrc = srcDef.attributes[k];

					attribDesc.binding = (uint32_t)i;
					attribDesc.format = yaptFormatToVk(attributeSrc.format);
					attribDesc.offset = attributeSrc.perVertexOffset;

					
					attribDesc.location = getInputLocationForSemantic(attributeSrc.shaderInputSlot, vertexShader->m_inputAttributes);
					assert(attribDesc.location != -1);
				}
			}

			vertexInputInfo.pVertexAttributeDescriptions = attributeDescs.data();
			vertexInputInfo.pVertexBindingDescriptions = bindingDescs.data();
			vertexInputInfo.vertexAttributeDescriptionCount = (uint32_t)attributeDescs.size();
			vertexInputInfo.vertexBindingDescriptionCount = (uint32_t)bindingDescs.size();
		}


		//viewport
		VkPipelineViewportStateCreateInfo viewPortInfo;
		std::vector<VkViewport> viewPorts;
		std::vector<VkRect2D> scissors;
		{
			viewPorts.resize(desc.numberOfViewportsAndScissors);
			scissors.resize(desc.numberOfViewportsAndScissors);

			for (size_t i = 0; i < desc.numberOfViewportsAndScissors; ++i)
			{
				yaptViewportToVk(desc.viewports[i], viewPorts[i]);
				yaptScissorsToVk(desc.scissors[i], scissors[i]);
			}

			viewPortInfo = {};
			viewPortInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
			viewPortInfo.pScissors = scissors.data();
			viewPortInfo.pViewports = viewPorts.data();
			viewPortInfo.scissorCount = (uint32_t)scissors.size();
			viewPortInfo.viewportCount = (uint32_t)viewPorts.size();
		}

		//misc
		VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo;
		VkPipelineTessellationStateCreateInfo tesselationInfo;
		VkPipelineDynamicStateCreateInfo dynamicState;
		std::vector<VkDynamicState> dynamicStates;
		{
			inputAssemblyInfo = {};
			inputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
			inputAssemblyInfo.topology = yaptPrimitiveTopologyToVk(desc.primitivetopology);
			inputAssemblyInfo.primitiveRestartEnable = false;
			inputAssemblyInfo.flags = 0;

			tesselationInfo = {};
			tesselationInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO;
			tesselationInfo.patchControlPoints = desc.tesselationPatchControlPointCount;
			tesselationInfo.flags = 0;


			dynamicState = {};
			dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
			dynamicState.flags = 0;			
			dynamicStates.resize(getDynamicStatesCount(desc.dynamicPipelineStates));
			yaptDynamicPipelineStateToVk(desc.dynamicPipelineStates, dynamicStates.data());
			dynamicState.pDynamicStates = dynamicStates.data();
			dynamicState.dynamicStateCount = (uint32_t)dynamicStates.size();
		}


		createInfo.pRasterizationState = &rasterInfo;
		createInfo.pMultisampleState = &multiSampleInfo;
		createInfo.pDepthStencilState = &dsvInfo;
		createInfo.pColorBlendState = &colorBlendInfo;
		createInfo.pVertexInputState = &vertexInputInfo;
		createInfo.pInputAssemblyState = &inputAssemblyInfo;
		createInfo.pTessellationState = &tesselationInfo;
		createInfo.pDynamicState = &dynamicState;
		createInfo.pViewportState = &viewPortInfo;


		VkResult res = vkCreateGraphicsPipelines(renderer->getDevice(), VK_NULL_HANDLE, 1, &createInfo, VK_ALLOC_CB, &pipeline);
		assert(res == VK_SUCCESS);
		return pipeline;
	}

	void GraphicsPipelineStateVk::destroy(RendererVk* renderer, VkPipeline pipeline)
	{
		renderer->getResourceManager()->deferredDestroyVkResource(pipeline);
	}
}