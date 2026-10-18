#include <Renderer/Shared/Utility/RenderResourcesPool.h>
#include <string.h>

namespace YAPT
{
	static bool isSameTextureDesc(const TextureDesc& a, const TextureDesc& b)
	{
		bool same = a.dimension == b.dimension
			&& a.format == b.format
			&& a.resourceUsage == b.resourceUsage
			&& a.width == b.width
			&& a.height == b.height
			&& a.mips == b.mips
			&& a.depthOrSlices == b.depthOrSlices
			&& a.memoryType == b.memoryType
			&& a.useOptimizedClearValue == b.useOptimizedClearValue;

		if (same && a.useOptimizedClearValue)
		{
			same = memcmp(&a.optimizedClearValue, &b.optimizedClearValue, sizeof(ClearValue)) == 0;
		}
		return same;
	}

	static bool isSameBufferDesc(const BufferDesc& a, const BufferDesc& b)
	{
		return a.resourceUsage == b.resourceUsage
			&& a.sizeInBytes == b.sizeInBytes
			&& a.memoryType == b.memoryType;
	}

	RenderResourcesPool::RenderResourcesPool(GfxApiHandle apiHandle)
		:m_apiHandle(apiHandle),
		m_inGeneration(false)
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
		if (name == nullptr) name = "unnamed texture";

		if (m_inGeneration)
		{
			for (TextureEntry& entry : m_textures)
			{
				if (!entry.claimed && entry.name == name && isSameTextureDesc(entry.desc, desc))
				{
					entry.claimed = true;
					return entry.handle;
				}
			}
		}

		TextureHandle texHandle =  Gfx::createTexture(m_apiHandle, desc, initialState, name);
		m_textures.push_back({ texHandle, desc, name, true });
		return texHandle;
	}
	BufferHandle RenderResourcesPool::requestBuffer(const YAPT::BufferDesc& desc, const ResourceStateDescription& initialState, const char* name)
	{
		if (name == nullptr) name = "unnamed buffer";

		if (m_inGeneration)
		{
			for (BufferEntry& entry : m_buffers)
			{
				if (!entry.claimed && entry.name == name && isSameBufferDesc(entry.desc, desc))
				{
					entry.claimed = true;
					return entry.handle;
				}
			}
		}

		BufferHandle buffHandle = Gfx::createBuffer(m_apiHandle, desc, initialState, name);
		m_buffers.push_back({ buffHandle, desc, name, true });
		return buffHandle;
	}

	void RenderResourcesPool::beginGeneration()
	{
		assert(!m_inGeneration);
		m_inGeneration = true;

		for (TextureEntry& entry : m_textures)
		{
			entry.claimed = false;
		}
		for (BufferEntry& entry : m_buffers)
		{
			entry.claimed = false;
		}
	}

	void RenderResourcesPool::endGeneration()
	{
		assert(m_inGeneration);
		m_inGeneration = false;

		size_t texturesKept = 0;
		for (size_t i = 0; i < m_textures.size(); ++i)
		{
			if (m_textures[i].claimed)
			{
				m_textures[texturesKept++] = std::move(m_textures[i]);
			}
			else
			{
				Gfx::destroyTexture(m_apiHandle, m_textures[i].handle);
			}
		}
		m_textures.resize(texturesKept);

		size_t buffersKept = 0;
		for (size_t i = 0; i < m_buffers.size(); ++i)
		{
			if (m_buffers[i].claimed)
			{
				m_buffers[buffersKept++] = std::move(m_buffers[i]);
			}
			else
			{
				Gfx::destroyBuffer(m_apiHandle, m_buffers[i].handle);
			}
		}
		m_buffers.resize(buffersKept);
	}

	void RenderResourcesPool::deallocate()
	{
		for (size_t i = 0; i < m_textures.size(); ++i)
		{
			Gfx::destroyTexture(m_apiHandle, m_textures[i].handle);
		}

		for (size_t i = 0; i < m_buffers.size(); ++i)
		{
			Gfx::destroyBuffer(m_apiHandle, m_buffers[i].handle);
		}

		m_textures.clear();
		m_buffers.clear();
		m_inGeneration = false;
	}
}