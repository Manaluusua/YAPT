#pragma once

#include <AssetLoader/AssetLoader.h>

namespace YAPT
{
	

	class CAssetLoader : public AssetLoader
	{
	public:
		CAssetLoader();
		virtual TextureLoadingContext* loadTexture(const char* path) final;
		virtual SceneLoadingContext* loadScene(const char* path) final;
		virtual void storeTexture(const char* path, const TextureStoreInfo& info) final;
		virtual ~CAssetLoader() final;
	protected:
		
	};
}