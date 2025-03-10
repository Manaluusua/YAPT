#ifndef YAPT_SHARED_TEXTUREIMPL_H
#define YAPT_SHARED_TEXTUREIMPL_H

#include <Renderer/Texture.h>
#include <Gfx/GfxTypes.h>
namespace YAPT
{
	class CRenderer;
	class TextureImpl : public Texture
	{
	public:
		TextureImpl(const TextureDesc texDesc, CRenderer* p);
		virtual ~TextureImpl();

		//Texture
		virtual void upload(uint32_t mipOffset, uint32_t arrayOffset, uint32_t mipCount, uint32_t arrayCount, const TextureDataDefinition* textureDataDefinitions);
		virtual const TextureDesc& getDesc() const { return m_desc; }

		virtual void setName(const char* name) { m_name = name; };
		virtual const char* getName() const { return m_name.c_str(); }

		void setResourceHandle(TextureHandle texHandle);
		TextureHandle getResourceHandle() const;

		void setBindlessResourceArrayIndex(uint32_t index) { m_bindlessArrayIndex = index; }
		uint32_t getBindlessResourceArrayIndex() const { return m_bindlessArrayIndex; }

	protected:
		CRenderer* m_renderer;
		TextureDesc m_desc;
		TextureHandle m_texHandle;
		uint32_t m_bindlessArrayIndex;
		std::string m_name;
	};
}

#endif