#include <Renderer/Vk/RenderNodeVk.h>

namespace YAPT
{



	RenderNodeVk::RenderNodeVk(const char* name, RenderGraph* graph, size_t numberOfConnectionSlots, const RenderGraphNodeSlotDefinition* slotDefinitions)
		:RenderNode(name, graph, numberOfConnectionSlots, slotDefinitions)
	{
		size_t clearValueCount = 0;
		size_t numberOfAttachments = getNumberOfColorTargets() + (hasDepthStencil() ? 1 : 0);
		m_clearValues.resize(numberOfAttachments);
		for (size_t i = 0; i < numberOfAttachments; ++i)
		{
			size_t slotIndex;
			bool isDepthStencil = (i == (numberOfAttachments - 1)) && hasDepthStencil();
			if (isDepthStencil)
			{
				slotIndex = getNodeSlotIndexForDepthStencil();
			}
			else
			{
				slotIndex = getNodeSlotIndexForRenderTargetIndex(i);
			}
			
			ClearValue val = slotDefinitions[slotIndex].clearValue;
			VkClearValue valVk = {};

			switch (val.type)
			{
			case ClearValue::_ClearValueType::FLOAT:
				memcpy(&valVk.color.float32, &val.value.fvec, sizeof(float) * 4);
				break;
			case ClearValue::_ClearValueType::UINT:
				memcpy(&valVk.color.uint32, &val.value.uvec, sizeof(uint32_t) * 4);
				break;
			case ClearValue::_ClearValueType::DEPTH_STENCIL:
				valVk.depthStencil.depth = val.value.depthStencil.depth;
				valVk.depthStencil.stencil = val.value.depthStencil.stencil;
				break;
			default:
				break;
			}

			m_clearValues[i] = valVk;
		}



	}
	RenderNodeVk::~RenderNodeVk()
	{

	}

	RenderPassHandle RenderNodeVk::getRenderPassHandle()
	{
		return &m_renderPassHandle;
	}

}