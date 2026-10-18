#ifndef YAPT_SHARED_RENDERRESOURCESPOOL_H
#define YAPT_SHARED_RENDERRESOURCESPOOL_H

#include <Gfx/GfxApi.h>
#include <Common/CommonUtilities.h>
#include <vector>
#include <string>

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

		//requests between begin and end reuse unclaimed resources with the same name and desc, end destroys the unclaimed ones
		void beginGeneration();
		void endGeneration();

		void deallocate();

		GfxApiHandle getGfxHandle() const { return m_apiHandle; }

	private:
		struct TextureEntry
		{
			TextureHandle handle;
			TextureDesc desc;
			std::string name;
			bool claimed;
		};

		struct BufferEntry
		{
			BufferHandle handle;
			BufferDesc desc;
			std::string name;
			bool claimed;
		};

		GfxApiHandle m_apiHandle;
		
		std::vector<TextureEntry> m_textures;
		std::vector<BufferEntry> m_buffers;
		bool m_inGeneration;

	};
}
#endif