#include <Renderer/Shared/BufferImpl.h>
#include <Renderer/Shared/CRenderer.h>
namespace YAPT
{

	BufferImpl::BufferImpl(const BufferDesc& desc, CRenderer* p)
		:m_desc(desc),
		m_renderer(p),
		m_bufferHandle(YAPT_NULL_HANDLE),
		m_bindlessArrayIndex(0)
	{

	}
	BufferImpl::~BufferImpl()
	{
		if (m_bufferHandle != YAPT_NULL_HANDLE)
		{
			m_renderer->bufferReleased(this);
			Gfx::destroyBuffer(m_renderer->getGfxHandle(), m_bufferHandle);
			m_bufferHandle = YAPT_NULL_HANDLE;
		}
	}

	void BufferImpl::setResourceHandle(BufferHandle bufferHandle)
	{
		m_bufferHandle = bufferHandle;
	}

	void BufferImpl::upload(size_t offsetInBytes, size_t sizeInBytes, const void* data)
	{
		Gfx::uploadBuffer(m_renderer->getGfxHandle(), m_bufferHandle, offsetInBytes, sizeInBytes, data, GpuUploadStage::BEFORE_RENDER);
	}

	BufferHandle BufferImpl::getResourceHandle() const
	{
		return m_bufferHandle;
	}

}
