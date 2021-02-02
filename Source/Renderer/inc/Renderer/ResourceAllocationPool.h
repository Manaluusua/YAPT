#ifndef YAPT_RESOURCECHUNK_H
#define YAPT_RESOURCECHUNK_H

#include "RendererCommonTypes.h"
#include <Common/RCObject.h>
#include <Renderer/Texture.h>
#include <Renderer/Buffer.h>

namespace YAPT
{

	class ResourceAllocationPool
	{
	public:
		
		virtual Texture* addTexture(const char* name, ResourceDimension dimensions, ResourceFormat format, ResourceUsage resourceUsage, uint32_t width, uint32_t height, uint32_t mips = 1, uint32_t depthOrSlices = 1) = 0;
		virtual Buffer* addBuffer(const char* name, ResourceUsage resourceUsage, size_t size) = 0;

		/*after this call, all the textures and buffers will have memory properly allocated and the app can update their data etc. Note that this will also consume the ResourceAllocationPool and no more
		resources can be allocated from this pool*/
		virtual void allocateAndConsume() = 0;


	protected:

		virtual ~ResourceAllocationPool() {}
	};
}

#endif
