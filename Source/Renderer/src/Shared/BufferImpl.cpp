#include <Renderer/Shared/BufferImpl.h>
#include <Renderer/Shared/ResourceAllocationPoolImpl.h>
#include <Renderer/Shared/CRenderer.h>
namespace YAPT
{

	BufferImpl::BufferImpl(const BufferDesc& desc, ResourceAllocationPoolImpl* pool)
		:m_desc(desc),
		m_resourceAllocationPool(pool),
		m_bufferHandle(YAPT_NULL_HANDLE),
		m_bindlessArrayIndex(0)
	{

	}
	BufferImpl::~BufferImpl()
	{
		m_resourceAllocationPool->bufferReleased(this);
	}

	void BufferImpl::setResourceHandle(BufferHandle bufferHandle)
	{
		m_bufferHandle = bufferHandle;
	}

	void BufferImpl::upload(size_t offsetInBytes, size_t sizeInBytes, const void* data)
	{
		Gfx::uploadBuffer(m_resourceAllocationPool->getRenderer()->getGfxHandle(), m_bufferHandle, offsetInBytes, sizeInBytes, data, GpuUploadStage::BEFORE_RENDER);
	}

	BufferHandle BufferImpl::getResourceHandle() const
	{
		return m_bufferHandle;
	}

}
