#include <AssetLoader/Impl/CDefaultRendererCacheProvider.h>
#include <AssetLoader/Impl/CAssetLoader.h>
#include <Gfx/GfxBasicTypesUtility.h>
#include <string>
namespace YAPT
{

	CDefaultRendererCacheProvider::CDefaultRendererCacheProvider()
	{

	}
	CDefaultRendererCacheProvider::~CDefaultRendererCacheProvider()
	{

	}

	bool CDefaultRendererCacheProvider::loadImage(const char* name, const ImageDefinition& expectedImageDefinition, void* dataOut)
	{
		std::string nameWithPostfix(name);
		nameWithPostfix += ".ktx";

		TextureLoadingContext* context = getAssetLoader()->loadTexture(nameWithPostfix.c_str());
		if (!context)
		{
			return false;
		}

		const TextureLoadInfo& loadInfo = context->getTextureInfo();

		bool success = false;

		if (loadInfo.dimension == expectedImageDefinition.dimension && loadInfo.format == expectedImageDefinition.format && loadInfo.numberOfMips == expectedImageDefinition.mips)
		{
			if (expectedImageDefinition.dimension == ResourceDimension::TEXTURE_3D || expectedImageDefinition.depthOrSlices == loadInfo.numberOfSlices)
			{
				uint32_t sliceCount = expectedImageDefinition.dimension == ResourceDimension::TEXTURE_3D ? 1 : expectedImageDefinition.depthOrSlices;
				size_t formatSize = getFormatSizeInBytes(loadInfo.format);
				
				char* currDataPtr = static_cast<char*>(dataOut);
				for (uint32_t slice = 0; slice < sliceCount; ++slice)
				{
					for (uint32_t mip = 0; mip < expectedImageDefinition.mips; ++mip)
					{
						const SubTextureLoadInfo& subTexInfo = context->getSubTextureInfo(mip, slice);
						for (uint32_t depthSlice = 0; depthSlice < subTexInfo.depth; ++depthSlice)
						{
							size_t footPrint = subTexInfo.rowPitch * subTexInfo.height;

							memcpy(currDataPtr, context->getData(mip, slice + depthSlice), footPrint);
							currDataPtr += footPrint;
						}
					}
				}

				success = true;
			}

		}


		context->release();

		return success;
	}
	void CDefaultRendererCacheProvider::storeImage(const char* name, const ImageDefinition& def, const void* data)
	{

		std::string nameWithPostfix(name);
		nameWithPostfix += ".ktx";


		TextureStoreInfo info;
		info.width = def.width;
		info.height = def.height;
		info.depth = def.dimension == ResourceDimension::TEXTURE_3D ? def.depthOrSlices : 1;
		info.numberOfSlices = def.dimension != ResourceDimension::TEXTURE_3D ? def.depthOrSlices : 1;
		info.dimension = def.dimension;
		info.format = def.format;
		info.numberOfMips = def.mips;

		assert(info.numberOfSlices == 1 && info.numberOfMips == 1); //not implemented yet for multiple slices/mips
		info.dataPtrs = &data;
		getAssetLoader()->storeTexture(nameWithPostfix.c_str(), info);
	}


	
}