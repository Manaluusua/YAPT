#include <Renderer/Shared/SceneInstanceData.h>
#include <Renderer/Shared/CRenderer.h>
#include <Renderer/Shared/BindlessMaterialManager.h>
#include <Renderer/Shared/BindlessMeshManager.h>
#include <Math/MathUtility.h>

namespace YAPT
{
	SceneInstanceData::SceneInstanceData()
		:m_renderer(nullptr),
		m_materialMngr(nullptr),
		m_meshMngr(nullptr),
		m_materialAndMeshIndices(RESOURCE_USAGE_COPY_DESTINATION | RESOURCE_USAGE_STORAGE_BUFFER, true),
		m_transformData(RESOURCE_USAGE_COPY_DESTINATION | RESOURCE_USAGE_STORAGE_BUFFER)
	{

	}

	SceneInstanceData::~SceneInstanceData()
	{

	}

	void SceneInstanceData::init(CRenderer* rend, BindlessMaterialManager* matMngr, BindlessMeshManager* meshMngr)
	{
		m_renderer = rend;
		m_materialMngr = matMngr;
		m_meshMngr = meshMngr;

		m_materialAndMeshIndices.init(m_renderer->getGfxHandle());
		m_transformData.init(m_renderer->getGfxHandle());
	}

	void SceneInstanceData::shutdown()
	{
		m_materialAndMeshIndices.free();
		m_transformData.free();
		m_instanceOffsetPerRenderObject.clear();
	}

	size_t SceneInstanceData::computeInstanceOffsets(CRenderer* rend, std::vector<size_t>& offsetsOut)
	{
		RenderObjectManager& roMngr = rend->getRenderObjectManager();
		MeshManager& meshMngr = rend->getMeshManager();

		MeshIndex* meshes = roMngr.getAllMeshes();
		size_t entryCount = roMngr.getNumberOfObjects();

		offsetsOut.resize(entryCount);

		size_t instanceCount = 0;
		for (size_t i = 0; i < entryCount; ++i)
		{
			offsetsOut[i] = instanceCount;
			instanceCount += meshMngr.getMeshInternal(meshes[i])->getSubmeshCount();
		}
		return instanceCount;
	}

	SceneInstanceData::SceneData SceneInstanceData::gatherSceneData(CRenderer* rend, const std::vector<size_t>& instanceOffsets, size_t instanceCount)
	{
		RenderObjectManager& roMngr = rend->getRenderObjectManager();

		SceneData sceneData;
		sceneData.ids = roMngr.getAllIds();
		sceneData.materials = roMngr.getAllMaterials();
		sceneData.meshes = roMngr.getAllMeshes();
		sceneData.instanceOffsetPerRenderObject = instanceOffsets.data();
		sceneData.objectCount = roMngr.getNumberOfObjects();
		sceneData.instancesCount = instanceCount;
		sceneData.objToWorldMatrices = roMngr.getAllWorldMatrices();
		sceneData.worldToObjMatrices = roMngr.getAllWorldInverseMatrices();
		return sceneData;
	}

	void SceneInstanceData::update(const SceneData& sceneData)
	{
		bool emptyScene = sceneData.instancesCount == 0;
		size_t numberOfEntriesNeeded = max((size_t)1, sceneData.instancesCount);
		MeshManager& meshMngr = m_renderer->getMeshManager();

		m_materialAndMeshIndices.allocate(numberOfEntriesNeeded, "RenderObjectsMaterialAndMeshIndicesBuffer");
		m_transformData.allocate(numberOfEntriesNeeded, "RenderObjectsTransformsBuffer");
		char* matAndMeshData = m_materialAndMeshIndices.map(0, numberOfEntriesNeeded);
		char* transformsData = m_transformData.map(0, numberOfEntriesNeeded);
		m_instanceOffsetPerRenderObject.resize(sceneData.objectCount);

		if (!emptyScene)
		{
			for (size_t i = 0; i < sceneData.objectCount; ++i)
			{
				MeshInternal* mesh = meshMngr.getMeshInternal(sceneData.meshes[i]);
				size_t entryOffset = sceneData.instanceOffsetPerRenderObject[i];
				m_instanceOffsetPerRenderObject[i] = entryOffset;

				//glm keeps things in column major order, tranpose to get rows out easily
				mat4 wMatT = glm::transpose(sceneData.objToWorldMatrices[i]);
				mat4 wMatInvT = glm::transpose(sceneData.worldToObjMatrices[i]);

				RenderObjectTransformDataGPU transformDataGPU;
				transformDataGPU.objToWorldR0 = wMatT[0];
				transformDataGPU.objToWorldR1 = wMatT[1];
				transformDataGPU.objToWorldR2 = wMatT[2];
				transformDataGPU.worldToObjectR0 = wMatInvT[0];
				transformDataGPU.worldToObjectR1 = wMatInvT[1];
				transformDataGPU.worldToObjectR2 = wMatInvT[2];

				for (size_t k = 0; k < mesh->getSubmeshCount(); ++k)
				{
					size_t instanceIndex = entryOffset + k;
					const MaterialPerSubmeshArray& mat = sceneData.materials[i];

					size_t matId = mat.getMaterialIDForSubmeshIndex(k);
					size_t meshId = mesh->getID();

					size_t matIndex = m_materialMngr->getEntryIndexForMaterialId(matId);
					size_t meshIndex = m_meshMngr->getEntryIndexForMeshIdAndSubmesh(meshId, k);

					uvec2p materialAndMeshIndices = uvec2p(matIndex, meshIndex);

					memcpy(matAndMeshData + (instanceIndex) * sizeof(uvec2p), &materialAndMeshIndices, sizeof(uvec2p));
					memcpy(transformsData + (instanceIndex) * sizeof(RenderObjectTransformDataGPU), &transformDataGPU, sizeof(RenderObjectTransformDataGPU));
				}
			}
		}
		else
		{
			memset(matAndMeshData, 0, sizeof(uvec2p));
			memset(transformsData, 0, sizeof(RenderObjectTransformDataGPU));
		}

		m_materialAndMeshIndices.unmap();
		m_transformData.unmap();
	}
}
