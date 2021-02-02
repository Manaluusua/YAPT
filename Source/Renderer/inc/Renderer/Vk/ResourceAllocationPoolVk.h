#ifndef YAPT_RESOURCEALLOCPOOL_VK_H
#define YAPT_RESOURCEALLOCPOOL_VK_H

#include <Renderer/Shared/GfxTypes.h>
#include <Renderer/Vk/ResourceManagerVk.h>
#include <vector>
namespace YAPT
{

	class ResourceAllocationPoolVk 
	{

		friend class TextureVk;
		friend class BufferVk;

	public:
		ResourceAllocationPoolVk(ResourceManagerVk& mngr);
		~ResourceAllocationPoolVk();

		bool allocate(TextureHandle* textures, size_t textureCount, BufferHandle* buffers, size_t bufferCount, ResourcePoolType type);

	private:
		ResourceManagerVk& m_resourceMngr;
		std::vector<ResourceManagerVk::AllocatedMemoryInfo> m_allocatedMemory;
		bool m_memoryMapped;

	};
}


#endif