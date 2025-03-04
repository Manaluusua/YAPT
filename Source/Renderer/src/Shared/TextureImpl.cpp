#include <Renderer/Shared/TextureImpl.h>
#include <Renderer/Shared/CRenderer.h>
namespace YAPT
{

	TextureImpl::TextureImpl(const TextureDesc texDesc, CRenderer* p)
		:m_desc(texDesc),
		m_renderer(p),
		m_texHandle(YAPT_NULL_HANDLE),
		m_bindlessArrayIndex(uint32_t(-1))
	{

	}
	TextureImpl::~TextureImpl()
	{
		if (m_texHandle != YAPT_NULL_HANDLE)
		{
			m_renderer->textureReleased(this);
			Gfx::destroyTexture(m_renderer->getGfxHandle(), m_texHandle);
			m_texHandle = YAPT_NULL_HANDLE;
		}
		
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
		Gfx::uploadTexture(m_renderer->getGfxHandle(), m_texHandle, arrayOffset, arrayCount, mipOffset, mipCount, textureDataDefinitions, GpuUploadStage::BEFORE_RENDER);
	}

}
