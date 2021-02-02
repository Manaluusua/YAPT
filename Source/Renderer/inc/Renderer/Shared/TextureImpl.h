#ifndef YAPT_SHARED_TEXTUREIMPL_H
#define YAPT_SHARED_TEXTUREIMPL_H

#include <Renderer/Texture.h>
#include <Renderer/Shared/GfxTypes.h>
namespace YAPT
{
	class ResourceAllocationPoolImpl;
	class TextureImpl : public Texture
	{
	public:
		TextureImpl(ResourceDimension d, ResourceFormat f, Flags u, uint32_t w, uint32_t h, uint32_t m, uint32_t s, ResourceAllocationPoolImpl* p);
		virtual ~TextureImpl();

		//Texture
		virtual void upload(uint32_t mipOffset, uint32_t arrayOffset, uint32_t mipCount, uint32_t arrayCount, const TextureDataDefinition* textureDataDefinitions);
		virtual const TextureDesc& getDesc() const { return m_desc; }

		void setResourceHandle(TextureHandle texHandle);
		TextureHandle getResourceHandle() const;

		void setBindlessResourceArrayIndex(uint32_t index) { m_bindlessArrayIndex = index; }
		uint32_t getBindlessResourceArrayIndex() const { return m_bindlessArrayIndex; }

	protected:
		ResourceAllocationPoolImpl* m_resourceAllocationPool;
		TextureDesc m_desc;
		TextureHandle m_texHandle;
		uint32_t m_bindlessArrayIndex;
	};
}

#endif