#include <Renderer/Shared/RenderPipeline/Realtime/RealtimeSceneResources.h>
#include <Renderer/Shared/CRenderer.h>
#include <Renderer/Shared/BindlessMaterialManager.h>
#include <Renderer/Shared/BindlessMeshManager.h>
#include <Math/MathUtility.h>
#include <algorithm>

namespace YAPT
{
	RealtimeSceneResources::RealtimeSceneResources()
		:m_renderer(nullptr),
		m_matMngr(nullptr),
		m_meshMngr(nullptr),
		m_drawInstanceIndices(RESOURCE_USAGE_COPY_DESTINATION | RESOURCE_USAGE_VERTEX_BUFFER | RESOURCE_USAGE_STORAGE_BUFFER, false, false),
		m_drawArgs(RESOURCE_USAGE_COPY_DESTINATION | RESOURCE_USAGE_INDIRECT_ARGUMENT_BUFFER | RESOURCE_USAGE_STORAGE_BUFFER, true, false),
		m_firstUpdate(true)
	{

	}

	RealtimeSceneResources::~RealtimeSceneResources()
	{

	}

	void RealtimeSceneResources::init(CRenderer* rend, RenderResourcesPool* pipelineLifetimeResources, BindlessMaterialManager* matMngr, BindlessMeshManager* meshMngr)
	{
		m_renderer = rend;
		m_matMngr = matMngr;
		m_meshMngr = meshMngr;

		m_instanceData.init(rend, matMngr, meshMngr);
		m_viewConstants.init(pipelineLifetimeResources, "RealtimeViewConstants");
		m_drawInstanceIndices.init(rend->getGfxHandle());
		m_drawArgs.init(rend->getGfxHandle());

		m_drawBuckets.clear();
		m_firstUpdate = true;
	}

	void RealtimeSceneResources::shutdown()
	{
		m_instanceData.shutdown();
		m_drawInstanceIndices.free();
		m_drawArgs.free();
		m_drawBuckets.clear();
	}

	bool RealtimeSceneResources::hasSceneChanged() const
	{
		return m_renderer->getMaterialManager().hasChanges() || m_renderer->getMeshManager().hasChanges() || m_renderer->getRenderObjectManager().hasChanges();
	}

	void RealtimeSceneResources::update(uvec2 renderResolution)
	{
		if (m_firstUpdate || hasSceneChanged())
		{
			rebuildDraws();
			m_firstUpdate = false;
		}

		RealtimeViewConstants* constants = m_viewConstants.getData();
		constants->worldToClip = m_renderer->getCurrentRenderView().getViewProjectionPlatform();
		constants->renderResolution = vec4p((float)renderResolution.x, (float)renderResolution.y, 1.f / (float)renderResolution.x, 1.f / (float)renderResolution.y);
		m_viewConstants.flush();
	}

	void RealtimeSceneResources::rebuildDraws()
	{
		size_t instanceCount = SceneInstanceData::computeInstanceOffsets(m_renderer, m_instanceOffsetPerRenderObject);
		SceneInstanceData::SceneData sceneData = SceneInstanceData::gatherSceneData(m_renderer, m_instanceOffsetPerRenderObject, instanceCount);
		m_instanceData.update(sceneData);

		struct InstanceKey
		{
			uint64_t key;
			uint32_t instanceIndex;
			uint32_t indexCount;
		};

		std::vector<InstanceKey> keys;
		keys.reserve(instanceCount);

		MeshManager& meshMngr = m_renderer->getMeshManager();
		for (size_t i = 0; i < sceneData.objectCount; ++i)
		{
			MeshInternal* mesh = meshMngr.getMeshInternal(sceneData.meshes[i]);
			for (size_t k = 0; k < mesh->getSubmeshCount(); ++k)
			{
				uint64_t meshEntry = m_meshMngr->getEntryIndexForMeshIdAndSubmesh(mesh->getID(), k);
				uint64_t matEntry = m_matMngr->getEntryIndexForMaterialId(sceneData.materials[i].getMaterialIDForSubmeshIndex(k));

				InstanceKey entry;
				entry.key = (meshEntry << 32) | (matEntry & 0xFFFFFFFF);
				entry.instanceIndex = (uint32_t)(m_instanceOffsetPerRenderObject[i] + k);
				entry.indexCount = (uint32_t)mesh->getSubmesh(k).indexCount;
				keys.push_back(entry);
			}
		}

		std::sort(keys.begin(), keys.end(), [](const InstanceKey& a, const InstanceKey& b)
			{
				return a.key != b.key ? a.key < b.key : a.instanceIndex < b.instanceIndex;
			});

		std::vector<DrawIndirectArguments> args;
		m_drawBuckets.clear();
		for (size_t i = 0; i < keys.size(); ++i)
		{
			if (m_drawBuckets.empty() || keys[i].key != keys[i - 1].key)
			{
				DrawBucket bucket;
				bucket.meshEntryIndex = (uint32_t)(keys[i].key >> 32);
				bucket.materialEntryIndex = (uint32_t)(keys[i].key & 0xFFFFFFFF);
				bucket.firstInstance = (uint32_t)i;
				bucket.instanceCount = 0;
				m_drawBuckets.push_back(bucket);

				DrawIndirectArguments arg;
				arg.vertexCount = keys[i].indexCount;
				arg.instanceCount = 0;
				arg.firstVertex = 0;
				arg.firstInstance = (uint32_t)i;
				args.push_back(arg);
			}
			m_drawBuckets.back().instanceCount += 1;
			args.back().instanceCount += 1;
		}

		size_t instanceEntries = max((size_t)1, keys.size());
		if (m_drawInstanceIndices.getAllocatedEntryCount() < instanceEntries)
		{
			m_drawInstanceIndices.allocate(instanceEntries, "RealtimeDrawInstanceIndices");
		}
		size_t argEntries = max((size_t)1, args.size());
		if (m_drawArgs.getAllocatedEntryCount() < argEntries)
		{
			m_drawArgs.allocate(argEntries, "RealtimeDrawArguments");
		}

		if (keys.empty()) return;

		char* instanceDst = m_drawInstanceIndices.map(0, keys.size());
		for (size_t i = 0; i < keys.size(); ++i)
		{
			memcpy(instanceDst + i * m_drawInstanceIndices.getAlignedEntrySize(), &keys[i].instanceIndex, sizeof(uint32_t));
		}
		m_drawInstanceIndices.unmap();

		char* argsDst = m_drawArgs.map(0, args.size());
		for (size_t i = 0; i < args.size(); ++i)
		{
			memcpy(argsDst + i * m_drawArgs.getAlignedEntrySize(), &args[i], sizeof(DrawIndirectArguments));
		}
		m_drawArgs.unmap();
	}
}
