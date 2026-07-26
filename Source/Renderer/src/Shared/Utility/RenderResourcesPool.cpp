#include <Renderer/Shared/Utility/RenderResourcesPool.h>

namespace YAPT
{
	RenderResourcesPool::RenderResourcesPool(GfxApiHandle apiHandle)
		:m_apiHandle(apiHandle)
	{

	}
	RenderResourcesPool::~RenderResourcesPool()
	{

	}

	TextureHandle RenderResourcesPool::requestTexture(const YAPT::TextureDesc& desc, const char* name)
	{
		return requestTexture(desc, ResourceStateDescription::defaultInitialResourceState(), name);
	}
	BufferHandle RenderResourcesPool::requestBuffer(const YAPT::BufferDesc& desc, const char* name)
	{
		return requestBuffer(desc, ResourceStateDescription::defaultInitialResourceState(), name);
	}

	TextureHandle RenderResourcesPool::requestTexture(const YAPT::TextureDesc& desc, const ResourceStateDescription& initialState, const char* name)
	{
		TextureHandle texHandle =  Gfx::createTexture(m_apiHandle, desc, initialState, name);
		m_textures.push_back(texHandle);
		return texHandle;
	}
	BufferHandle RenderResourcesPool::requestBuffer(const YAPT::BufferDesc& desc, const ResourceStateDescription& initialState, const char* name)
	{
		BufferHandle buffHandle = Gfx::createBuffer(m_apiHandle, desc, initialState, name);
		m_buffers.push_back(buffHandle);
		return buffHandle;
	}

	void RenderResourcesPool::deallocate()
	{
		for (size_t i = 0; i < m_textures.size(); ++i)
		{
			Gfx::destroyTexture(m_apiHandle, m_textures[i]);
		}

		for (size_t i = 0; i < m_buffers.size(); ++i)
		{
			Gfx::destroyBuffer(m_apiHandle, m_buffers[i]);
		}

		m_textures.clear();
		m_buffers.clear();
	}
}