#include <Renderer/Shared/BindlessMeshManager.h>
#include <Renderer/Shared/CRenderer.h>

#include <Math/MathUtility.h>
#include <Gfx/GfxBasicTypesUtility.h>
namespace YAPT
{
	size_t MESH_BUFFER_INITIAL_SIZE = 128;
	uvec2p packBufferInfo(uint32_t bufferIndex, uint32_t bufferStride, uint32_t bufferOffset)
	{
		glm::uvec2 v;
		v.x = bufferOffset;
		v.y = bufferStride << 16 | (bufferIndex & 0xFFFF);
		return v;
	}


	BindlessMeshManager::BindlessMeshManager() 
		:m_renderer(nullptr),
		m_gpuBuffer(ResourceUsageBits::RESOURCE_USAGE_COPY_DESTINATION | ResourceUsageBits::RESOURCE_USAGE_STORAGE_BUFFER)
	{

	}
	BindlessMeshManager::~BindlessMeshManager()
	{
		shutdown();
	}

	void BindlessMeshManager::init(CRenderer* renderer)
	{
		m_renderer = renderer;
		m_gpuBuffer.init(renderer->getGfxHandle());
		m_gpuBuffer.allocate(MESH_BUFFER_INITIAL_SIZE, "MeshesBuffer");

		syncMeshStateToGPU();
	}
	void BindlessMeshManager::update()
	{
		MeshManager& meshManager = m_renderer->getMeshManager();
		if (meshManager.hasChanges())
		{
			
			syncMeshStateToGPU();
		}
	}

	void BindlessMeshManager::syncMeshStateToGPU()
	{
		MeshManager& meshManager = m_renderer->getMeshManager();
		//for now just rewrite everything on any change. TODO: do subupdates if this grows too big
		size_t sizeNeeded = meshManager.getTotalSubmeshCount();
		if (m_gpuBuffer.getAllocatedEntryCount() < sizeNeeded)
		{
			m_gpuBuffer.free();
			m_gpuBuffer.allocate(sizeNeeded, "MeshesBuffer");
		}

		if (m_meshIDToBufferIndex.size() <= meshManager.getHighestAllocatedIndex())
		{
			m_meshIDToBufferIndex.clear();
			m_meshIDToBufferIndex.resize(meshManager.getHighestAllocatedIndex() + 1, -1);
		}

		if (sizeNeeded == 0) return;

		auto getPackedBufferInfo = [](const MeshLayoutInfo& info, const AttributeMapping& attrMapping, const MeshInternal* mesh, const SubmeshDefinition& sm)
		{
			glm::uvec2 retVal;
			if (attrMapping.bufferIndex != uint32_t(-1))
			{

				const MeshBufferBinding& bufferBinding = mesh->getVertexBuffer(attrMapping.bufferIndex);
				uint32_t bufferIndex = bufferBinding.buffer->getBindlessResourceArrayIndex();
				size_t vertexBufferOffset = bufferBinding.offsetInBytes;
				uint32_t stride = info.vertexBufferConfigurations[attrMapping.bufferIndex].stride;
				size_t offset = vertexBufferOffset + sm.vertexOffset * stride + info.vertexBufferConfigurations[attrMapping.bufferIndex].offsetFromVertexStart[attrMapping.attributeIndex];

				//make sure we can fit the data in packing
				assert(offset < 0xFFFFFFFF);
				assert(bufferIndex < 0xFFFF);
				assert(stride < 0xFFFF);

				//from byteoffset to float/uint offset
				retVal = packBufferInfo(bufferIndex, stride / 4, (uint32_t)offset / 4);
			}
			else
			{
				retVal = glm::uvec2(-1, -1);
			}
			return retVal;
		};

		char* gpuBuffer = m_gpuBuffer.map(0, sizeNeeded);

		auto& iter = meshManager.getMeshIterator();
		size_t index = 0;
		MeshInternal* mesh = iter.getCurrent();
		while (mesh != nullptr)
		{

			m_meshIDToBufferIndex[mesh->getMeshIndex()] = index;

			for (size_t i = 0; i < mesh->getSubmeshCount(); ++i)
			{
				MeshEntryGPU dst;

				const SubmeshDefinition& sm = mesh->getSubmesh(i);
				//index buffer
				{
					const MeshBufferBinding& indexBufferBinding = mesh->getIndexBuffer();

					bool use16BitIndices = mesh->getIndexBufferFormat() == ResourceFormat::R16_UINT;
					size_t offsetInBytes = indexBufferBinding.offsetInBytes + sm.indexOffset * getFormatSizeInBytes(mesh->getIndexBufferFormat());
					uint8_t extraOffset = offsetInBytes % 4;
					uint8_t stride = use16BitIndices ? 2 : 3;
					dst.indexBuffer = packBufferInfo(indexBufferBinding.buffer->getBindlessResourceArrayIndex(), (extraOffset << 8) | stride, uint32_t(offsetInBytes) / sizeof(uint32_t)); //16bit indices marked with stride of 2. If you change this, remember to change this assumptin in the shaders too!
				}
				//vertex data
				{
					const MeshLayoutInfo& info = mesh->getLayoutInfo();

					dst.positionBuffer = getPackedBufferInfo(info, info.position0, mesh, sm);
					dst.normalBuffer = getPackedBufferInfo(info, info.normal0, mesh, sm);
					dst.tangentBuffer = getPackedBufferInfo(info, info.tangent0, mesh, sm);
					dst.uvBuffer = getPackedBufferInfo(info, info.uv0, mesh, sm);
				}

				memcpy(gpuBuffer + m_gpuBuffer.getAlignedEntrySize() * index, &dst, sizeof(MeshEntryGPU));

				++index;
			}
			mesh = iter.getNextValidEntry();
		}

		m_gpuBuffer.unmap();
	}

	void BindlessMeshManager::shutdown()
	{
		m_gpuBuffer.free();
	}
}