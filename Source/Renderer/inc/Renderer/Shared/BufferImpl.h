#ifndef YAPT_SHARED_BUFFERIMPL_H
#define YAPT_SHARED_BUFFERIMPL_H

#include <Renderer/Buffer.h>
#include <Renderer/Shared/GfxTypes.h>

namespace YAPT
{
	class ResourceAllocationPoolImpl;
	class BufferImpl : public Buffer
	{
	public:
		BufferImpl(Flags usage, size_t size, ResourceAllocationPoolImpl* pool);
		virtual ~BufferImpl();

		//Buffer
		void upload(size_t offsetInBytes, size_t sizeInBytes, const void* data);
		virtual const BufferDesc& getDesc() const { return m_desc; }

		void setResourceHandle(BufferHandle bufferHandle);
		BufferHandle getResourceHandle() const;

		void setBindlessResourceArrayIndex(uint32_t index) { m_bindlessArrayIndex = index; }
		uint32_t getBindlessResourceArrayIndex() const { return m_bindlessArrayIndex; }

	protected:
		ResourceAllocationPoolImpl* m_resourceAllocationPool;
		BufferDesc m_desc;
		BufferHandle m_bufferHandle;
		uint32_t m_bindlessArrayIndex;
	};
}

#endif