#ifndef YAPT_SHARED_BUFFERIMPL_H
#define YAPT_SHARED_BUFFERIMPL_H

#include <Renderer/Buffer.h>
#include <Gfx/GfxTypes.h>

namespace YAPT
{
	class CRenderer;
	class BufferImpl : public Buffer
	{
	public:
		BufferImpl(const BufferDesc& desc, CRenderer* p);
		virtual ~BufferImpl();

		//Buffer
		void upload(size_t offsetInBytes, size_t sizeInBytes, const void* data);
		virtual const BufferDesc& getDesc() const { return m_desc; }

		virtual void setName(const char* name) { m_name = name; };
		virtual const char* getName() const { return m_name.c_str(); }

		void setResourceHandle(BufferHandle bufferHandle);
		BufferHandle getResourceHandle() const;

		void setBindlessResourceArrayIndex(uint32_t index) { m_bindlessArrayIndex = index; }
		uint32_t getBindlessResourceArrayIndex() const { return m_bindlessArrayIndex; }

	protected:
		CRenderer* m_renderer;
		BufferDesc m_desc;
		BufferHandle m_bufferHandle;
		uint32_t m_bindlessArrayIndex;
		std::string m_name;
	};
}

#endif