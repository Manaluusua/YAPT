#pragma once

#include <AssetLoader/AssetLoader.h>
#include <ktx.h>
#include <vector>
namespace YAPT
{
	class CKtxTexture : public TextureLoadingContext
	{
	public:
		CKtxTexture();
		~CKtxTexture();

		bool init(const char* path);

		virtual const TextureLoadInfo& getTextureInfo() final;
		virtual const SubTextureLoadInfo& getSubTextureInfo(uint32_t mip, uint32_t arraySlice) final;
		virtual const void* getData(uint32_t mip, uint32_t arraySliceOrDepth) final;
		virtual void release() final;

		static void storeTexture(const char* path, const TextureStoreInfo& info);
	private:
		ktxTexture* m_tex;
		TextureLoadInfo m_texInfo;
		std::vector<SubTextureLoadInfo> m_subTextureInfo;
	};

	
}