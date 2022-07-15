#include <Renderer/Shared/ResourceAllocationPoolImpl.h>
#include <Renderer/Shared/TextureImpl.h>
#include <Renderer/Shared/BufferImpl.h>
#include <Renderer/Shared/CRenderer.h>
#include <Common/CommonUtilities.h>
#include <assert.h>

namespace YAPT
{

	ResourceAllocationPoolImpl::ResourceAllocationPoolImpl(CRenderer* renderer)
		:m_renderer(renderer),
		m_allocated(false),
		m_resourcesAlive(0)
	{

	}
	ResourceAllocationPoolImpl::~ResourceAllocationPoolImpl()
	{

	}


	Texture* ResourceAllocationPoolImpl::addTexture(const char* name, ResourceDimension dimensions, ResourceFormat format, ResourceUsage resourceUsage, uint32_t width, uint32_t height, uint32_t mips, uint32_t depthOrSlices)
	{
		assert(m_allocated == false);
		m_resourcesAlive.fetch_add(1);
		TextureDesc desc(dimensions, format, resourceUsage, width, height, mips, depthOrSlices);
		m_renderer->textureToBeCreated(desc);
		TextureImpl* tex = new TextureImpl(desc, this);
		ResourceStateDescription state{ RESOURCE_USAGE_COPY_DESTINATION, ACCESS_FLAGS_WRITE, SHADERSTAGE_NONE };
		TextureHandle texHandle = Gfx::createTexture(m_renderer->getGfxHandle(), tex->getDesc(), state, name);
		tex->setResourceHandle(texHandle);
		m_textures.push_back(tex);

		return tex;
	}
	Buffer* ResourceAllocationPoolImpl::addBuffer(const char* name, ResourceUsage usageFlags, size_t size)
	{
		assert(m_allocated == false);
		m_resourcesAlive.fetch_add(1);
		BufferDesc desc(usageFlags, size);
		m_renderer->bufferToBeCreated(desc);
		BufferImpl* buf = new BufferImpl(desc, this);
		ResourceStateDescription state{ RESOURCE_USAGE_COPY_DESTINATION, ACCESS_FLAGS_WRITE, SHADERSTAGE_NONE };
		BufferHandle bufHandle = Gfx::createBuffer(m_renderer->getGfxHandle(), buf->getDesc(), state, name);
		buf->setResourceHandle(bufHandle);
		m_buffers.push_back(buf);

		

		return buf;
	}

	void ResourceAllocationPoolImpl::allocateAndConsume()
	{
		assert(m_allocated == false);
		allocate();
		m_allocated = true;

		//notify renderer of the created textures and buffers
		for (TextureImpl* tex : m_textures)
		{
			m_renderer->textureCreated(tex);
		}

		for (BufferImpl* buff : m_buffers)
		{
			m_renderer->bufferCreated(buff);
		}

		//in corner case where nothing was allocated, just destruct
		resourceReleased();
	}

	void ResourceAllocationPoolImpl::bufferReleased(BufferImpl* buff)
	{
		assert(m_resourcesAlive.load() > 0);
		m_resourcesAlive.fetch_add(-1);

		m_renderer->bufferReleased(buff);

		Gfx::destroyBuffer(m_renderer->getGfxHandle(), buff->getResourceHandle());
		resourceReleased();
	}

	void ResourceAllocationPoolImpl::textureReleased(TextureImpl* tex)
	{
		assert(m_resourcesAlive.load() > 0);
		m_resourcesAlive.fetch_add(-1);

		m_renderer->textureReleased(tex);

		Gfx::destroyTexture(m_renderer->getGfxHandle(), tex->getResourceHandle());
		resourceReleased();
	}

	void ResourceAllocationPoolImpl::resourceReleased()
	{
		assert(m_allocated && "Tried to release resource when it wasnt even allocated");
		if (m_resourcesAlive.load() == 0)
		{
			destroy();
		}

	}


	void ResourceAllocationPoolImpl::allocate()
	{
		std::vector<BufferHandle> bufferHandles;
		std::vector<TextureHandle> textureHandles;
		bufferHandles.reserve(m_buffers.size());
		textureHandles.reserve(m_textures.size());

		for (size_t i = 0; i < m_buffers.size(); ++i)
		{
			bufferHandles.push_back(m_buffers[i]->getResourceHandle());
		}

		for (size_t i = 0; i < m_textures.size(); ++i)
		{
			textureHandles.push_back(m_textures[i]->getResourceHandle());
		}

		m_chunkHandle = Gfx::createResourcePool(m_renderer->getGfxHandle(), textureHandles.data(), textureHandles.size(), bufferHandles.data(), bufferHandles.size(), ResourcePoolType::RESOURCEPOOL_TYPE_DEFAULT);

	}
	void ResourceAllocationPoolImpl::destroy()
	{
		assert(m_allocated == true);

		Gfx::destroyResourcePool(m_renderer->getGfxHandle(), m_chunkHandle);

		m_textures.clear();
		m_buffers.clear();

		delete this;
	}

}


