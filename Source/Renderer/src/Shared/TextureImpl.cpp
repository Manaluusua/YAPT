#include <Renderer/Shared/TextureImpl.h>
#include <Renderer/Shared/ResourceAllocationPoolImpl.h>
#include <Renderer/Shared/CRenderer.h>
namespace YAPT
{

	TextureImpl::TextureImpl(const TextureDesc texDesc, ResourceAllocationPoolImpl* p)
		:m_desc(texDesc),
		m_resourceAllocationPool(p),
		m_texHandle(YAPT_NULL_HANDLE),
		m_bindlessArrayIndex(uint32_t(-1))
	{

	}
	TextureImpl::~TextureImpl()
	{
		m_resourceAllocationPool->textureReleased(this);
	}

	void TextureImpl::setResourceHandle(TextureHandle texHandle)
	{
		m_texHandle = texHandle;

	}
	TextureHandle TextureImpl::getResourceHandle() const
	{
		return m_texHandle;
	}

	void TextureImpl::upload(uint32_t mipOffset, uint32_t arrayOffset, uint32_t mipCount, uint32_t arrayCount, const TextureDataDefinition* textureDataDefinitions)
	{
		Gfx::uploadTexture(m_resourceAllocationPool->getRenderer()->getGfxHandle(), m_texHandle, arrayOffset, arrayCount, mipOffset, mipCount, textureDataDefinitions, GpuUploadStage::BEFORE_RENDER);
	}

}
