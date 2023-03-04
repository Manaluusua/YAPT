#include <Renderer/Shared/RenderPipeline/RenderStageUtilities.h>
#include <Renderer/Shared/RenderGraph/RenderGraph.h>
namespace YAPT
{
	void DeferredRenderGraphBindingUtility::setDeferredRenderGraphResourceBinding(RenderGraphResourceId id, TextureHandle handle)
	{
		m_deferredRenderGraphResourceBindings.resize(m_deferredRenderGraphResourceBindings.size() + 1);
		DeferredRenderGraphResourceBinding& binding = m_deferredRenderGraphResourceBindings.back();
		binding.id = id;
		binding.type = DeferredRenderGraphResourceBinding::TEXTURE;
		binding.texture = handle;
	}
	void DeferredRenderGraphBindingUtility::setDeferredRenderGraphResourceBinding(RenderGraphResourceId id, BufferHandle handle)
	{
		m_deferredRenderGraphResourceBindings.resize(m_deferredRenderGraphResourceBindings.size() + 1);
		DeferredRenderGraphResourceBinding& binding = m_deferredRenderGraphResourceBindings.back();
		binding.id = id;
		binding.type = DeferredRenderGraphResourceBinding::BUFFER;
		binding.buffer = handle;
	}

	void DeferredRenderGraphBindingUtility::flushDeferredRenderGraphResourceBindings(RenderGraph* graph)
	{
		for (size_t i = 0; i < m_deferredRenderGraphResourceBindings.size(); ++i)
		{
			const DeferredRenderGraphResourceBinding& bindings = m_deferredRenderGraphResourceBindings[i];
			if (bindings.type == DeferredRenderGraphResourceBinding::TEXTURE)
			{
				graph->setRenderGraphResourceTexture(bindings.id, bindings.texture);
			}
			else
			{
				graph->setRenderGraphResourceBuffer(bindings.id, bindings.buffer);
			}
		}

		m_deferredRenderGraphResourceBindings.clear();
	}

}