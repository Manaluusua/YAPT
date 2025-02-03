#ifndef YAPT_SHARED_RESOURCECHUNKIMPL_H
#define YAPT_SHARED_RESOURCECHUNKIMPL_H

#include <Renderer/ResourceAllocationPool.h>
#include <Gfx/GfxApi.h>
#include <vector>

namespace YAPT
{
	class TextureImpl;
	class BufferImpl;
	class CRenderer;
	class ResourceAllocationPoolImpl : public ResourceAllocationPool
	{
		friend class TextureImpl;
		friend class BufferImpl;

	public:
		ResourceAllocationPoolImpl(CRenderer* renderer);
		virtual ~ResourceAllocationPoolImpl();

		//ResourceChunk
		virtual Texture* addTexture(const char* name, ResourceDimension dimensions, ResourceFormat format, ResourceUsage resourceUsage, uint32_t width, uint32_t height, uint32_t mips, uint32_t depthOrSlices) override;
		virtual Buffer* addBuffer(const char* name, ResourceUsage resourceUsage, size_t size) override;
		virtual void allocateAndConsume() override;

		CRenderer* getRenderer() const { return m_renderer; }

	protected:
		void allocate();
		void destroy();

		void bufferReleased(BufferImpl* buff);
		void textureReleased(TextureImpl* tex);
		void resourceReleased();

		CRenderer* m_renderer;
		ResourceAllocationPoolHandle m_chunkHandle;
		std::vector<TextureImpl*> m_textures;
		std::vector<BufferImpl*> m_buffers;

		bool m_allocated;
		std::atomic<size_t> m_resourcesAlive;
	};
}

#endif