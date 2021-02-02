
#include <AssetLoader/Impl/CAssetLoader.h>
#include <AssetLoader/Impl/CTextureHandler.h>
#include <AssetLoader/Impl/CSceneLoader.h>
#include <AssetLoader/Impl/CDefaultRendererCacheProvider.h>
namespace YAPT
{
	ASSETLOADER_MODULE_INTERFACE AssetLoader* getAssetLoader()
	{
		static CAssetLoader loader;
		return &loader;
	}

	RendererCacheProvider* getDefaultRendererCacheProvider()
	{
		static CDefaultRendererCacheProvider provider;
		return &provider;
	}

	CAssetLoader::CAssetLoader()
	{

	}
	CAssetLoader::~CAssetLoader()
	{

	}
	TextureLoadingContext* CAssetLoader::loadTexture(const char* path)
	{
		CKtxTexture* loader = new CKtxTexture();
		if (!loader->init(path))
		{
			delete loader;
			return nullptr;
		}

		return loader;
	}

	SceneLoadingContext* CAssetLoader::loadScene(const char* path)
	{
		CSceneLoader* loader = new CSceneLoader();
		if (!loader->init(path))
		{
			delete loader;
			return nullptr;
		}

		return loader;
	}


	void CAssetLoader::storeTexture(const char* path, const TextureStoreInfo& info)
	{
		CKtxTexture::storeTexture(path, info);
	}
	


}