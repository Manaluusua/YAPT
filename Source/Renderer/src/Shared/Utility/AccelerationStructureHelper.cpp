#include <Renderer/Shared/Utility/AccelerationStructureHelper.h>
#include <Renderer/Shared/CRenderer.h>
#include <Renderer/Shared/MeshInternal.h>
#include <Gfx/GfxBasicTypesUtility.h>

namespace YAPT
{

	void fillGeometryDefinitions(const MeshInternal* mesh, AccelerationStructureGeometryDefinition* targetDef)
	{
		const MeshLayoutInfo& info = mesh->getLayoutInfo();
		const VertexBufferConfiguration& vertexBufferConfig = info.vertexBufferConfigurations[info.position0.bufferIndex];
		const MeshBufferBinding& vBuffer = mesh->getVertexBuffer(info.position0.bufferIndex);

		for (size_t i = 0; i < mesh->getSubmeshCount(); ++i)
		{
			AccelerationStructureGeometryDefinition& geom = targetDef[i];
			const SubmeshDefinition& sm = mesh->getSubmesh(i);
			geom.vertexCount = info.numberOfVertices - sm.vertexOffset;
			geom.vertexStrideInBytes = vertexBufferConfig.stride;
			geom.vertexBufferOffsetInBytes = vBuffer.offsetInBytes + sm.vertexOffset * vertexBufferConfig.stride + vertexBufferConfig.offsetFromVertexStart[info.position0.attributeIndex];
			geom.vertexFormat = vertexBufferConfig.attributes[info.position0.attributeIndex].format;
			geom.indexCount = sm.indexCount;
			geom.indexBufferOffsetInBytes = sm.indexOffset * getFormatSizeInBytes(mesh->getIndexBufferFormat());
			geom.indexFormat = mesh->getIndexBufferFormat();

			geom.vertexBuffer = vBuffer.buffer->getResourceHandle();
			geom.indexBuffer = mesh->getIndexBuffer().buffer->getResourceHandle();

			geom.worldMatrixBuffer = YAPT_NULL_HANDLE;
			geom.worldMatrixBufferOffsetInBytes = 0;
		}

	}

	void AccelerationStructureHelper::fillInstanceTransform(const mat4& src, float dst[12])
	{

		dst[0] = src[0][0];
		dst[1] = src[1][0];
		dst[2] = src[2][0];
		dst[3] = src[3][0];

		dst[4] = src[0][1];
		dst[5] = src[1][1];
		dst[6] = src[2][1];
		dst[7] = src[3][1];

		dst[8] = src[0][2];
		dst[9] = src[1][2];
		dst[10] = src[2][2];
		dst[11] = src[3][2];
	}


	AccelerationStructureHelper::AccelerationStructureHelper()
		:m_renderer(nullptr),
		m_isFirstUpdate(true),
		m_tlasHandle(YAPT_NULL_HANDLE)
	{
		m_blasArrayPerMesh.resize(256);
	}
	AccelerationStructureHelper::~AccelerationStructureHelper()
	{
		deinit();
	}

	void AccelerationStructureHelper::deinit()
	{
		m_temporaryContainers.clear();

		for (size_t i = 0; i < m_blasArrayPerMesh.size(); ++i)
		{
			MeshBLASArray& arr = m_blasArrayPerMesh[i];
			arr.pushToArrayAndClear(m_temporaryContainers.blasHandlesToRelease);
		}
		if (m_temporaryContainers.blasHandlesToRelease.size() > 0)
		{
			Gfx::destroyBottomLevelAccelerationStructures(m_renderer->getGfxHandle(), m_temporaryContainers.blasHandlesToRelease.data(), m_temporaryContainers.blasHandlesToRelease.size());
		}

		if (m_tlasHandle != YAPT_NULL_HANDLE)
		{
			Gfx::destroyTopLevelAccelerationStructures(m_renderer->getGfxHandle(), &m_tlasHandle, 1);
			m_tlasHandle = YAPT_NULL_HANDLE;
		}

	}

	void AccelerationStructureHelper::init(CRenderer* renderer)
	{
		m_renderer = renderer;
		m_tlasHandle = YAPT_NULL_HANDLE;
	}

	void AccelerationStructureHelper::updateBottomLevelStructures(CommandBufferHandle commandBuffer)
	{

		m_blasHadChanges = false;

		//make sure our internal structure can hold all the potential blas handles
		MeshManager& mngr = m_renderer->getMeshManager();
		size_t requiredSize = mngr.getHighestAllocatedIndex() + 1;
		if (m_blasArrayPerMesh.size() < requiredSize)
		{
			m_blasArrayPerMesh.resize(requiredSize);
		}

		m_temporaryContainers.clear();

		//for now put all geometry to different BLAS structures. Later on could mark meshes that could be put to the same BLAS?
		std::vector<AccelerationStructureGeometryDefinition>& geomDefs = m_temporaryContainers.geomDefs;
		std::vector<MeshIndex>& meshIndicesWithBLASBuilt = m_temporaryContainers.meshIndexOfBlas;
		std::vector<BottomLevelAccelerationStructureDefinition>& blasDefs = m_temporaryContainers.blasDefs;
		std::vector<BottomLevelAccelerationStructureHandle>& blasHandles = m_temporaryContainers.blasHandles;
		std::vector<BottomLevelAccelerationStructureHandle>& blasHandlesToRelease = m_temporaryContainers.blasHandlesToRelease;



		if (m_isFirstUpdate)
		{
			m_blasArrayPerMesh.resize(m_renderer->getMeshManager().getHighestAllocatedIndex() + 1);
			MeshManager::MeshIterator iter = mngr.getMeshIterator();
			const MeshInternal* mesh = iter.getCurrent();
			
			while (mesh != nullptr)
			{
				size_t subMeshCount = mesh->getSubmeshCount();
				size_t offset = geomDefs.size();
				geomDefs.resize(geomDefs.size() + subMeshCount);
				fillGeometryDefinitions(mesh, geomDefs.data() + offset);
				MeshIndex meshIndex = mesh->getID();
				meshIndicesWithBLASBuilt.push_back(meshIndex);
				m_blasArrayPerMesh[meshIndex].resize(subMeshCount);

				mesh = iter.getNextValidEntry();
			}
		}
		else
		{
			//create new entries
			const MeshIndex* meshIndices;
			size_t numberOfIndices;
			mngr.getCreatedEntries(meshIndices, numberOfIndices);

			for (size_t i = 0; i < numberOfIndices; ++i)
			{
				MeshIndex meshIndex = meshIndices[i];
				const MeshInternal* mesh = mngr.getMeshInternal(meshIndex);
				size_t subMeshCount = mesh->getSubmeshCount();

				size_t offset = geomDefs.size();
				geomDefs.resize(geomDefs.size() + subMeshCount);
				fillGeometryDefinitions(mesh, geomDefs.data() + offset);
				meshIndicesWithBLASBuilt.push_back(meshIndex);
				m_blasArrayPerMesh[meshIndex].resize(subMeshCount);
			}

			//let go of old entries
			mngr.getDestroyedEntries(meshIndices, numberOfIndices);

			for (size_t i = 0; i < numberOfIndices; ++i)
			{
				MeshIndex meshIndex = meshIndices[i];

				MeshBLASArray& arr = m_blasArrayPerMesh[meshIndex];
				arr.pushToArrayAndClear(blasHandlesToRelease);

			}


			//update changed (Should probably check if the change actually affects the blas)
			mngr.getModifiedEntries(meshIndices, numberOfIndices);

			for (size_t i = 0; i < numberOfIndices; ++i)
			{
				MeshIndex meshIndex = meshIndices[i];

				//let go of old
				MeshBLASArray& arr = m_blasArrayPerMesh[meshIndex];
				arr.pushToArrayAndClear(blasHandlesToRelease);

				const MeshInternal* mesh = mngr.getMeshInternal(meshIndex);
				size_t subMeshCount = mesh->getSubmeshCount();

				size_t offset = geomDefs.size();
				geomDefs.resize(geomDefs.size() + subMeshCount);
				fillGeometryDefinitions(mesh, geomDefs.data() + offset);
				meshIndicesWithBLASBuilt.push_back(meshIndex);
				arr.resize(subMeshCount);

			}
		}

		if (blasHandlesToRelease.size() > 0)
		{
			Gfx::destroyBottomLevelAccelerationStructures(m_renderer->getGfxHandle(), blasHandlesToRelease.data(), blasHandlesToRelease.size());
			m_blasHadChanges = true;
		}

		if (geomDefs.size() > 0)
		{
			blasDefs.resize(geomDefs.size());
			for (size_t i = 0; i < geomDefs.size(); ++i)
			{
				blasDefs[i].geometryDefinitionCount = 1;
				blasDefs[i].geometryDefinitions = &geomDefs[i];
			}
			blasHandles.resize(blasDefs.size());

			//for now do allocation and build at the same place
			Gfx::allocateBottomLevelAccelerationStructures(m_renderer->getGfxHandle(), blasDefs.data(), blasDefs.size(), blasHandles.data());
			Gfx::buildBottomLevelAccelerationStructures(m_renderer->getGfxHandle(), commandBuffer,  blasHandles.data(), blasDefs.size());

			size_t blasOffset = 0;

			for (size_t i = 0; i < meshIndicesWithBLASBuilt.size(); ++i)
			{
				size_t meshIndex = meshIndicesWithBLASBuilt[i];
				blasOffset += m_blasArrayPerMesh[meshIndex].fill(blasHandles.data() + blasOffset);
			}

			m_blasHadChanges = true;

		}

	}

	


	bool AccelerationStructureHelper::doesTopLevelAccelerationStructureNeedRebuild()
	{
		if (m_tlasHandle == YAPT_NULL_HANDLE) return true;
		RenderObjectManager& mngr = m_renderer->getRenderObjectManager();
		bool renderObjectsHaveChanges = false;

		const RenderObjectId* id;
		size_t numEntries;
		mngr.getCreatedEntries(id, numEntries);
		if (numEntries > 0)
		{
			renderObjectsHaveChanges = true;
		}

		mngr.getDestroyedEntries(id, numEntries);
		if (numEntries > 0)
		{
			renderObjectsHaveChanges = true;
		}

		mngr.getModifiedEntries(id, numEntries);
		if (numEntries > 0)
		{
			renderObjectsHaveChanges = true;
		}

		return m_blasHadChanges || renderObjectsHaveChanges;
	}
}