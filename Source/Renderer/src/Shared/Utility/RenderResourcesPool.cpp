#include <Renderer/Shared/Utility/RenderResourcesPool.h>

namespace YAPT
{
	RenderResourcesPool::RenderResourcesPool(GfxApiHandle apiHandle)
		:m_apiHandle(apiHandle),
		m_resourceChunkHandle(YAPT_NULL_HANDLE)
	{

	}
	RenderResourcesPool::~RenderResourcesPool()
	{

	}

	TextureHandle RenderResourcesPool::requestTexture(const YAPT::TextureDesc& desc, const char* name)
	{
		return requestTexture(desc, ResourceStateDescription::default(), name);
	}
	BufferHandle RenderResourcesPool::requestBuffer(const YAPT::BufferDesc& desc, const char* name)
	{
		return requestBuffer(desc, ResourceStateDescription::default(), name);
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

	void RenderResourcesPool::allocate()
	{
		m_resourceChunkHandle = Gfx::createResourcePool(m_apiHandle, m_textures.data(), m_textures.size(), m_buffers.data(), m_buffers.size(), ResourcePoolType::RESOURCEPOOL_TYPE_DEFAULT);
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

		Gfx::destroyResourcePool(m_apiHandle, m_resourceChunkHandle);
		m_resourceChunkHandle = YAPT_NULL_HANDLE;
		m_textures.clear();
		m_buffers.clear();
	}
}