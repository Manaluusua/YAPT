#include <AssetLoader/Impl/CSceneLoader.h>
#include <Renderer/RendererCommonTypesUtility.h>
#include <string>
#include <string_view>
#include <unordered_map>

#define TINYGLTF_IMPLEMENTATION
#include <tiny_gltf.h>

namespace YAPT
{
	ResourceFormat getAttributeFormat(const tinygltf::Accessor& accessor)
	{
		int componentType = accessor.componentType;
		int type = accessor.type;

		switch (type)
		{
		case TINYGLTF_TYPE_VEC2:

			switch (componentType)
			{
			case TINYGLTF_COMPONENT_TYPE_BYTE:
				return ResourceFormat::RG8_SINT;
			case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
				return ResourceFormat::RG8_UINT;
			case TINYGLTF_COMPONENT_TYPE_SHORT:
				return ResourceFormat::RG16_SINT;
			case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
				return ResourceFormat::RG16_UINT;
			case TINYGLTF_COMPONENT_TYPE_INT:
				return ResourceFormat::RG32_SINT;
			case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
				return ResourceFormat::RG32_UINT;
			case TINYGLTF_COMPONENT_TYPE_FLOAT:
				return ResourceFormat::RG32_SFLOAT;
			case TINYGLTF_COMPONENT_TYPE_DOUBLE:
			default:
				return ResourceFormat::UNKNOWN;
			}
			break;
		case TINYGLTF_TYPE_VEC3:
			switch (componentType)
			{
			case TINYGLTF_COMPONENT_TYPE_INT:
				return ResourceFormat::RGB32_SINT;
			case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
				return ResourceFormat::RGB32_UINT;
			case TINYGLTF_COMPONENT_TYPE_FLOAT:
				return ResourceFormat::RGB32_SFLOAT;
			case TINYGLTF_COMPONENT_TYPE_BYTE:
			case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
			case TINYGLTF_COMPONENT_TYPE_SHORT:
			case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
			case TINYGLTF_COMPONENT_TYPE_DOUBLE:
			default:
				return ResourceFormat::UNKNOWN;
			}
			break;

		case TINYGLTF_TYPE_VEC4:
			switch (componentType)
			{
			case TINYGLTF_COMPONENT_TYPE_BYTE:
				return ResourceFormat::RGBA8_SINT;
			case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
				return ResourceFormat::RGBA8_UINT;
			case TINYGLTF_COMPONENT_TYPE_SHORT:
				return ResourceFormat::RGBA16_SINT;
			case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
				return ResourceFormat::RGBA16_UINT;
			case TINYGLTF_COMPONENT_TYPE_INT:
				return ResourceFormat::RGBA32_SINT;
			case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
				return ResourceFormat::RGBA32_UINT;
			case TINYGLTF_COMPONENT_TYPE_FLOAT:
				return ResourceFormat::RGBA32_SFLOAT;
			case TINYGLTF_COMPONENT_TYPE_DOUBLE:
			default:
				return ResourceFormat::UNKNOWN;
			}
			break;

		case TINYGLTF_TYPE_SCALAR:
			switch (componentType)
			{
			case TINYGLTF_COMPONENT_TYPE_BYTE:
				return ResourceFormat::R8_SINT;
			case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
				return ResourceFormat::R8_UINT;
			case TINYGLTF_COMPONENT_TYPE_SHORT:
				return ResourceFormat::R16_SINT;
			case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
				return ResourceFormat::R16_UINT;
			case TINYGLTF_COMPONENT_TYPE_INT:
				return ResourceFormat::R32_SINT;
			case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
				return ResourceFormat::R32_UINT;
			case TINYGLTF_COMPONENT_TYPE_FLOAT:
				return ResourceFormat::R32_SFLOAT;
			case TINYGLTF_COMPONENT_TYPE_DOUBLE:
			default:
				return ResourceFormat::UNKNOWN;
			}
			break;

		case TINYGLTF_TYPE_MAT2:
		case TINYGLTF_TYPE_MAT3:
		case TINYGLTF_TYPE_MAT4:
		case TINYGLTF_TYPE_MATRIX:
		default:
			return ResourceFormat::UNKNOWN;
		}
	}

	AttributeSemantic stringToAttributeSemantic(const std::string& str)
	{
		AttributeSemanticName semanticName = AttributeSemanticName::UNKNOWN;
		uint16_t semanticIndex = 0;

		size_t separatorIndex = str.find('_');

		std::string_view semanticStr;

		if (separatorIndex != std::string::npos)
		{
			semanticStr = std::string_view(str.c_str(), separatorIndex);
			size_t offset = separatorIndex + 1;
			std::string indexStr = std::string(str.c_str() + offset, str.size() - offset);
			semanticIndex = std::atoi(indexStr.c_str());
		}
		else
		{
			semanticStr = std::string_view(str.c_str(), str.size());
		}


		if (semanticStr == "POSITION")
		{
			semanticName = AttributeSemanticName::POSITION;
		} 
		else if (semanticStr == "NORMAL")
		{
			semanticName = AttributeSemanticName::NORMAL;
		}
		else if (semanticStr == "TANGENT")
		{
			semanticName = AttributeSemanticName::TANGENT;
		}
		else if (semanticStr == "TEXCOORD")
		{
			semanticName = AttributeSemanticName::TEXCOORD;
		}
		else if (semanticStr == "COLOR")
		{
			semanticName = AttributeSemanticName::COLOR;
		}
		
		assert(semanticName != AttributeSemanticName::UNKNOWN);

		AttributeSemantic semantic;
		semantic.set(semanticName, semanticIndex);
		return semantic;
	}


	


	bool loadImageCallback(tinygltf::Image*, const int, std::string*,
		std::string*, int, int,
		const unsigned char*, int, void*)
	{
		//TODO
		return true;
	}


	CSceneLoader::CSceneLoader()
	{

	}

	CSceneLoader::~CSceneLoader()
	{

	}

	bool CSceneLoader::init(const char* path)
	{
		std::string warning;
		std::string error;

		std::string filename(path);
		
		std::string ext = tinygltf::GetFilePathExtension(filename);

		tinygltf::TinyGLTF loader;

		loader.SetImageLoader(&loadImageCallback, this);

		if (ext == std::string("gltf"))
		{
			if (!loader.LoadASCIIFromFile(&m_model, &error, &warning, filename))
			{
				return false;
			}
		}
		else
		{
			if (!loader.LoadBinaryFromFile(&m_model, &error, &warning, filename))
			{
				return false;
			}

		}
		
		loadMeshes();

		
		return true;
	}

	size_t CSceneLoader::getNumberOfBuffers() const
	{
		return m_buffers.size();
	}
	const BufferLoadInfo& CSceneLoader::getBufferLoadInfo(size_t index)
	{
		return  m_buffers[index].info;
	}
	void* CSceneLoader::getBufferData(size_t index)
	{
		return m_buffers[index].buffer.data();
	}

	size_t CSceneLoader::getNumberOfMeshes() const
	{
		return m_meshes.size();
	}
	const MeshLoadInfo& CSceneLoader::getMeshLoadInfo(size_t index)
	{
		return m_meshes[index].info;
	}

	void CSceneLoader::loadMeshes()
	{
		size_t numberOfPrimitives = 0;
		

		for (size_t i = 0; i < m_model.meshes.size(); ++i)
		{
			tinygltf::Mesh& srcMesh = m_model.meshes[i];

			numberOfPrimitives += srcMesh.primitives.size();
		}

		m_meshes.resize(numberOfPrimitives);
		size_t meshIndex = 0;

		for (size_t m = 0; m < m_model.meshes.size(); ++m)
		{
			
			for (size_t p = 0; p < m_model.meshes[m].primitives.size(); ++p)
			{
				
				tinygltf::Mesh& mesh = m_model.meshes[m];
				tinygltf::Primitive& prim = mesh.primitives[p];

				MeshLoadInfoInternal& meshLoadInfo = m_meshes[meshIndex];
				generateMeshLoadInfo(mesh, prim, meshLoadInfo);
				
				++meshIndex;
			}
		}
	}


	void CSceneLoader::generateMeshLoadInfo(tinygltf::Mesh& mesh, tinygltf::Primitive& prim, MeshLoadInfoInternal& meshLoadInfoOut)
	{
		//for now separate position from other attributes and generate 2 buffers: one with positions, other with rest of the attributes
		std::pair<Attribute, int> positionToAccessorMapping;
		std::vector<std::pair<Attribute, int>> attributeToAccessorMapping;

		for (auto iter = prim.attributes.begin(); iter != prim.attributes.end(); ++iter)
		{
			AttributeSemantic semantic = stringToAttributeSemantic(iter->first);
			int accessorIndex = iter->second;
			const tinygltf::Accessor& accessor = m_model.accessors[accessorIndex];
			const tinygltf::BufferView& bufferView = m_model.bufferViews[accessor.bufferView];

			Attribute attribute;
			attribute.semantic = semantic;
			attribute.format = getAttributeFormat(accessor);
			assert(attribute.format != ResourceFormat::UNKNOWN);

			if (attribute.format == ResourceFormat::UNKNOWN)
			{
				continue;
			}

			if (semantic == AttributeSemantic(AttributeSemanticName::POSITION, 0))
			{
				positionToAccessorMapping = std::make_pair(attribute, accessorIndex);
			}
			else
			{
				attributeToAccessorMapping.push_back(std::make_pair(attribute, accessorIndex));
			}
		}

		assert(positionToAccessorMapping.second != -1);

		size_t vertexBufferCount = attributeToAccessorMapping.size() > 0 ? 2 : 1;

		meshLoadInfoOut.vertexBufferLayouts.resize(vertexBufferCount);
		meshLoadInfoOut.bindingOffsets.resize(vertexBufferCount, 0);
		meshLoadInfoOut.bufferIndices.resize(vertexBufferCount);

		meshLoadInfoOut.attributes.resize(attributeToAccessorMapping.size() + 1);

		size_t vertexCount = 0;

		//position buffer
		{
			meshLoadInfoOut.attributes[0] = positionToAccessorMapping.first;
			VertexBufferLayout& layout = meshLoadInfoOut.vertexBufferLayouts[0];
			layout.attributeCount = 1;
			layout.attributes = meshLoadInfoOut.attributes.data();
			layout.vertexStrideInBytes = VERTEX_STRIDE_TIGHTLY_PACKED;

			vertexCount = m_model.accessors[positionToAccessorMapping.second].count;
		}

		//rest of the attributes
		{
			VertexBufferLayout& layout = meshLoadInfoOut.vertexBufferLayouts[1];
			layout.attributeCount = (uint32_t)attributeToAccessorMapping.size();
			layout.attributes = meshLoadInfoOut.attributes.data() + 1;
			layout.vertexStrideInBytes = VERTEX_STRIDE_TIGHTLY_PACKED;
			for (size_t i = 0; i < attributeToAccessorMapping.size(); ++i)
			{
				meshLoadInfoOut.attributes[i + 1] = attributeToAccessorMapping[i].first;
				assert(vertexCount == m_model.accessors[attributeToAccessorMapping[i].second].count);
			}
		}
		
		meshLoadInfoOut.info.layouts = meshLoadInfoOut.vertexBufferLayouts.data();
		meshLoadInfoOut.info.vertexBufferBindingOffsets = meshLoadInfoOut.bindingOffsets.data();
		meshLoadInfoOut.info.vertexBufferIndices = meshLoadInfoOut.bufferIndices.data();
		meshLoadInfoOut.info.numberOfVertexBuffers = meshLoadInfoOut.vertexBufferLayouts.size();

		meshLoadInfoOut.info.vertexCount = vertexCount;
		meshLoadInfoOut.info.primitiveCount = m_model.accessors[prim.indices].count / 3; //assuming triangles

		//handle buffers
		size_t firstVertexBufferIndex = m_buffers.size();
		m_buffers.resize(firstVertexBufferIndex + vertexBufferCount + 1);

		for (size_t i = 0; i < vertexBufferCount; ++i)
		{
			meshLoadInfoOut.bufferIndices[i] = firstVertexBufferIndex + i;
		}

		meshLoadInfoOut.info.indexBufferIndex = m_buffers.size() - 1;


		fillVertexBuffer(&positionToAccessorMapping, 1, m_buffers[firstVertexBufferIndex]);
		if (vertexBufferCount > 1)
		{
			fillVertexBuffer(attributeToAccessorMapping.data(), attributeToAccessorMapping.size(), m_buffers[firstVertexBufferIndex + 1]);
		}
		fillIndexBuffer(prim.indices, m_buffers[meshLoadInfoOut.info.indexBufferIndex]);

	}


	struct BufferCopyEntry
	{
		size_t srcStride;
		size_t srcElementSize;
		size_t dstElementSize;
		
		const void* src;
	};

	void copyInterleaved(void* dst, size_t elementCount, const BufferCopyEntry* entries, size_t entryCount)
	{
		char* dest = static_cast<char*>(dst);

		for (size_t i = 0; i < elementCount; ++i)
		{
			for (size_t k = 0; k < entryCount; ++k)
			{
				const BufferCopyEntry& entry = entries[k];
				const char* source = static_cast<const char*>(entry.src);
				source += entry.srcStride * i;

				assert(entry.srcElementSize <= entry.dstElementSize);

				memcpy(dest, source, entries[k].srcElementSize);

				dest += entry.dstElementSize;
			}
		}
	}

	

	void CSceneLoader::fillVertexBuffer(std::pair<Attribute, int>* attributeAndAccessors, size_t count, BufferLoadInfoInternal& target)
	{
		size_t bufferSize = 0;
		size_t lastAttributeCount = 0;

		std::vector<BufferCopyEntry> copyEntries;
		copyEntries.resize(count);

		for (size_t i = 0; i < count; ++i)
		{
			BufferCopyEntry& copyEntry = copyEntries[i];
			std::pair<Attribute, int> attr = attributeAndAccessors[i];

			const tinygltf::Accessor& accessor = m_model.accessors[attr.second];
			const tinygltf::BufferView& buffView = m_model.bufferViews[accessor.bufferView];
			const tinygltf::Buffer& buffer = m_model.buffers[buffView.buffer];

			copyEntry.srcElementSize = getFormatSizeInBytes(attr.first.format);
			copyEntry.dstElementSize = copyEntry.srcElementSize;
			copyEntry.srcStride = buffView.byteStride == 0 ? copyEntry.srcElementSize : buffView.byteStride;
			copyEntry.src = buffer.data.data() + buffView.byteOffset + accessor.byteOffset;

			bufferSize += copyEntry.dstElementSize * accessor.count;
			if (i != 0)
			{
				assert(accessor.count == lastAttributeCount);
			}

			lastAttributeCount = accessor.count;
		}

		target.buffer.resize(bufferSize);
		copyInterleaved(target.buffer.data(), lastAttributeCount, copyEntries.data(), count);

		target.info.sizeInBytes = bufferSize;
		target.buffer.resize(bufferSize);

	}

	void CSceneLoader::fillIndexBuffer(int accessorIndex, BufferLoadInfoInternal& target)
	{

		BufferCopyEntry entry;


		const tinygltf::Accessor& accessor = m_model.accessors[accessorIndex];
		const tinygltf::BufferView& buffView = m_model.bufferViews[accessor.bufferView];
		const tinygltf::Buffer& buffer = m_model.buffers[buffView.buffer];

		ResourceFormat indexFormat = getAttributeFormat(accessor);

		bool uses32bitIndices = true;
		if (indexFormat == ResourceFormat::R32_UINT)
		{
			uses32bitIndices = true;
		}
		else if (indexFormat == ResourceFormat::R16_UINT)
		{
			uses32bitIndices = false;
		}
		else
		{
			assert(!"Incorrect index format!");
		}

		entry.srcElementSize = uses32bitIndices ? 4 : 2;
		entry.dstElementSize = getFormatSizeInBytes(ResourceFormat::R32_UINT);
		entry.srcStride = buffView.byteStride == 0 ? entry.srcElementSize : buffView.byteStride;
		entry.src = buffer.data.data() + buffView.byteOffset + accessor.byteOffset;

		size_t elementCount = accessor.count;
		size_t bufferSize = elementCount * entry.dstElementSize;

		target.buffer.resize(bufferSize);
		copyInterleaved(target.buffer.data(), elementCount, &entry, 1);

		target.info.sizeInBytes = bufferSize;
		target.buffer.resize(bufferSize);
	}

}