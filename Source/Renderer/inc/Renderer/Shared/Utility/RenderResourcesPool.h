#ifndef YAPT_SHARED_RENDERRESOURCESPOOL_H
#define YAPT_SHARED_RENDERRESOURCESPOOL_H

#include <Gfx/GfxApi.h>
#include <Common/CommonUtilities.h>
#include <vector>

namespace YAPT
{
	
	class RenderResourcesPool
	{
		YAPT_NOCOPY(RenderResourcesPool);
	public:
		RenderResourcesPool(GfxApiHandle apiHandle);
		~RenderResourcesPool();

		TextureHandle requestTexture(const YAPT::TextureDesc& desc,  const char* name = "unnamed texture");
		BufferHandle requestBuffer(const YAPT::BufferDesc& desc,const char* name = "unnamed buffer");
		
		TextureHandle requestTexture(const YAPT::TextureDesc& desc, const ResourceStateDescription& initialState, const char* name = "unnamed texture");
		BufferHandle requestBuffer(const YAPT::BufferDesc& desc, const ResourceStateDescription& initialState, const char* name = "unnamed buffer");

		void deallocate();

		GfxApiHandle getGfxHandle() const { return m_apiHandle; }

	private:
		GfxApiHandle m_apiHandle;
		
		std::vector<TextureHandle> m_textures;
		std::vector<BufferHandle> m_buffers;

	};
}
#endif