#pragma once

#include <AssetLoader/AssetLoader.h>
#include <vector>
#define TINYGLTF_NOEXCEPTION
#define TINYGLTF_NO_STB_IMAGE 
#define TINYGLTF_NO_STB_IMAGE_WRITE 
#include <tiny_gltf.h>
#include <AssetLoader/Impl/CTextureHandler.h>

namespace YAPT
{
	class CSceneLoader : public SceneLoadingContext
	{
	public:
		CSceneLoader();
		~CSceneLoader();
	
		bool init(const char* path);

		virtual size_t getNumberOfBuffers() const final;
		virtual const BufferLoadInfo& getBufferLoadInfo(size_t index) final;
		virtual void* getBufferData(size_t index) final;

		virtual size_t getNumberOfMeshes() const final;
		virtual const MeshLoadInfo& getMeshLoadInfo(size_t index) final;

	private:

		struct MeshLoadInfoInternal
		{
			MeshLoadInfo info;
			std::vector<Attribute> attributes;
			std::vector<VertexBufferLayout> vertexBufferLayouts;
			std::vector<size_t> bindingOffsets;
			std::vector<size_t> bufferIndices;

		};

		struct BufferLoadInfoInternal
		{
			BufferLoadInfo info;
			std::vector<uint8_t> buffer;
		};

		void loadMeshes();
		void generateMeshLoadInfo(tinygltf::Mesh& mesh, tinygltf::Primitive& prim, MeshLoadInfoInternal& meshLoadInfoOut);
		void fillVertexBuffer(std::pair<Attribute, int>* attributeAndAccessors, size_t count, BufferLoadInfoInternal& target);
		void fillIndexBuffer(int accessorIndex, BufferLoadInfoInternal& target);

		std::vector<MeshLoadInfoInternal> m_meshes;
		std::vector<BufferLoadInfoInternal> m_buffers;
		tinygltf::Model m_model;
	};
}