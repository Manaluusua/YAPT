#pragma once

#include <Gfx/GfxApi.h>
#include <Gfx/RenderGraph/RenderGraphCommon.h>

namespace YAPT
{
	class DeferredRenderGraphBindingUtility
	{
	public:
		void setDeferredRenderGraphResourceBinding(RenderGraphResourceId id, TextureHandle handle);
		void setDeferredRenderGraphResourceBinding(RenderGraphResourceId id, BufferHandle handle);

		void flushDeferredRenderGraphResourceBindings(RenderGraph* graph);
	private:
		struct DeferredRenderGraphResourceBinding
		{
			RenderGraphResourceId id;
			enum
			{
				BUFFER,
				TEXTURE
			} type;
			union
			{
				TextureHandle texture;
				BufferHandle buffer;
			};

		};

		std::vector<DeferredRenderGraphResourceBinding> m_deferredRenderGraphResourceBindings;
	};
	

}