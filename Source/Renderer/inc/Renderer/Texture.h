#ifndef YAPT_TEXTURE_H
#define YAPT_TEXTURE_H

#include <Gfx/GfxBasicTypes.h>
#include <Common/RCObject.h>
namespace YAPT
{

	
	class Texture : public RCObject
	{
	public:
		virtual void upload(uint32_t mipOffset, uint32_t arrayOffset, uint32_t mipCount, uint32_t arrayCount, const TextureDataDefinition* textureDataDefinitions) = 0;
		virtual const TextureDesc& getDesc() const = 0;
	protected:
		virtual ~Texture() {}
	};
}

#endif