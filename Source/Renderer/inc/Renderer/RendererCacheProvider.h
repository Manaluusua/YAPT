#pragma once

#include <Renderer/RendererCommonTypes.h>
#include <Math/Math.h>
namespace YAPT
{
	
	class RendererCacheProvider
	{
	public:

		struct ImageDefinition
		{
			ResourceDimension dimension;
			ResourceFormat format;
			uint32_t width;
			uint32_t height;
			uint32_t mips;
			uint32_t depthOrSlices;
		};

		virtual bool loadImage(const char* name, const ImageDefinition& expectedImageDefinition, void* dataOut) = 0;
		virtual void storeImage(const char* name, const ImageDefinition& def, const void* data) = 0;
	};
}