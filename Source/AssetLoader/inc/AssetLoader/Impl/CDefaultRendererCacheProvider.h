#pragma once

#include <Renderer/RendererCacheProvider.h>
#include <AssetLoader/AssetLoader.h>
namespace YAPT
{
	class CDefaultRendererCacheProvider : public RendererCacheProvider
	{
	public:
		CDefaultRendererCacheProvider();
		~CDefaultRendererCacheProvider();

		virtual bool loadImage(const char* name, const ImageDefinition& expectedImageDefinition, void* dataOut) override;
		virtual void storeImage(const char* name, const ImageDefinition& def, const void* data) override;

	
	};
}