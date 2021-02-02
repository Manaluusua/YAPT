#pragma once
#include"CommonDefines.h"
#include <Common/RCObject.h>
#include <Renderer/RendererCommonTypes.h>
#include <Renderer/Mesh.h>

namespace YAPT
{
	struct TextureLoadInfo
	{
		ResourceDimension dimension;
		uint32_t numberOfSlices;
		uint32_t numberOfMips;
		ResourceFormat format;
	};
	
	struct SubTextureLoadInfo
	{
		uint32_t width;
		uint32_t height;
		uint32_t depth;
		size_t rowPitch;
	};

	struct BufferLoadInfo
	{
		size_t sizeInBytes;
	};

	struct MeshLoadInfo
	{
		VertexBufferLayout* layouts;
		size_t* vertexBufferBindingOffsets;
		size_t* vertexBufferIndices;
		size_t numberOfVertexBuffers;

		size_t vertexCount;
		size_t primitiveCount;
		size_t indexBufferIndex;
	};

	struct TextureStoreInfo
	{
		uint32_t width;
		uint32_t height;
		uint32_t depth;
		ResourceDimension dimension;
		uint32_t numberOfSlices;
		uint32_t numberOfMips;
		ResourceFormat format;
		const void** dataPtrs; //order is index = mip + slice * mips
	};

	class AssetLoader;
	class RendererCacheProvider;

	ASSETLOADER_MODULE_INTERFACE AssetLoader* getAssetLoader();
	ASSETLOADER_MODULE_INTERFACE RendererCacheProvider* getDefaultRendererCacheProvider();

	class TextureLoadingContext
	{
	public:
		virtual const TextureLoadInfo& getTextureInfo() = 0;
		virtual const SubTextureLoadInfo& getSubTextureInfo(uint32_t mip, uint32_t arraySlice) = 0;
		virtual const void* getData(uint32_t mip, uint32_t arraySlice) = 0;
		virtual void release() = 0;
	};

	class SceneLoadingContext
	{
	public:
		virtual size_t getNumberOfBuffers() const = 0;
		virtual const BufferLoadInfo& getBufferLoadInfo(size_t index) = 0;
		virtual void* getBufferData(size_t index) = 0;

		virtual size_t getNumberOfMeshes() const = 0;
		virtual const MeshLoadInfo& getMeshLoadInfo(size_t index) = 0;

	};


	class AssetLoader
	{
	public:
		ASSETLOADER_MODULE_INTERFACE virtual TextureLoadingContext* loadTexture(const char* path) = 0;
		ASSETLOADER_MODULE_INTERFACE virtual SceneLoadingContext* loadScene(const char* path) = 0;
		ASSETLOADER_MODULE_INTERFACE virtual void storeTexture(const char* path, const TextureStoreInfo& info) = 0;
		virtual ~AssetLoader() {};

	};

	
}
