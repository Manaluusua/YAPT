#include <Renderer/Shared/Utility/AccelerationStructureHelper.h>
#include <Renderer/Shared/CRenderer.h>
#include <Renderer/Shared/MeshInternal.h>

namespace YAPT
{

	void fillGeometryDefinition(const MeshInternal* mesh, AccelerationStructureGeometryDefinition& geom)
	{
		const MeshLayoutInfo& info = mesh->getLayoutInfo();
		const VertexBufferConfiguration& vertexBufferConfig = info.vertexBufferConfigurations[info.position0.bufferIndex];
		const MeshBufferBinding& vBuffer = mesh->getVertexBuffer(info.position0.bufferIndex);


		geom.vertexCount = info.numberOfVertices;
		geom.vertexStrideInBytes = vertexBufferConfig.stride;
		geom.vertexBufferOffsetInBytes = vBuffer.offsetInBytes + vertexBufferConfig.offsetFromVertexStart[info.position0.attributeIndex];
		geom.vertexFormat = vertexBufferConfig.attributes[info.position0.attributeIndex].format;
		geom.indexCount = mesh->getPrimitiveCount() * 3; //Triangles
		geom.indexFormat = ResourceFormat::R32_UINT; //assume 32bit indices

		geom.vertexBuffer = vBuffer.buffer->getResourceHandle();
		geom.indexBuffer = mesh->getIndexBuffer().buffer->getResourceHandle();

		geom.worldMatrixBuffer = YAPT_NULL_HANDLE;
		geom.worldMatrixBufferOffsetInBytes = 0;
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
		m_blasPerMesh.resize(256, YAPT_NULL_HANDLE);
	}
	AccelerationStructureHelper::~AccelerationStructureHelper()
	{
		deinit();
	}

	void AccelerationStructureHelper::deinit()
	{
		m_temporaryContainers.clear();

		for (size_t i = 0; i < m_blasPerMesh.size(); ++i)
		{
			if (m_blasPerMesh[i] != YAPT_NULL_HANDLE)
			{
				m_temporaryContainers.blasHandlesToRelease.push_back(m_blasPerMesh[i]);
				m_blasPerMesh[i] = YAPT_NULL_HANDLE;
			}

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
		if (m_blasPerMesh.size() < requiredSize)
		{
			m_blasPerMesh.resize(requiredSize, YAPT_NULL_HANDLE);
		}

		m_temporaryContainers.clear();

		//for now put all geometry to different BLAS structures. Later on could mark meshes that could be put to the same BLAS?
		std::vector<AccelerationStructureGeometryDefinition>& geomDefs = m_temporaryContainers.geomDefs;
		std::vector<MeshIndex>& meshIndexOfBlas = m_temporaryContainers.meshIndexOfBlas;
		std::vector<BottomLevelAccelerationStructureDefinition>& blasDefs = m_temporaryContainers.blasDefs;
		std::vector<BottomLevelAccelerationStructureHandle>& blasHandles = m_temporaryContainers.blasHandles;
		std::vector<BottomLevelAccelerationStructureHandle>& blasHandlesToRelease = m_temporaryContainers.blasHandlesToRelease;


		if (m_isFirstUpdate)
		{
			m_blasPerMesh.resize(m_renderer->getMeshManager().getHighestAllocatedIndex() + 1);
			MeshManager::MeshIterator iter = mngr.getMeshIterator();
			const MeshInternal* mesh = iter.getCurrent();
			while (mesh != nullptr)
			{
				geomDefs.resize(geomDefs.size() + 1);
				AccelerationStructureGeometryDefinition& def = geomDefs.back();
				fillGeometryDefinition(mesh, def);
				MeshIndex meshIndex = mesh->getMeshIndex();;
				meshIndexOfBlas.push_back(meshIndex);

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
				geomDefs.resize(geomDefs.size() + 1);
				AccelerationStructureGeometryDefinition& def = geomDefs.back();
				fillGeometryDefinition(mesh, def);
				meshIndexOfBlas.push_back(meshIndex);
			}

			//let go of old entries
			mngr.getDestroyedEntries(meshIndices, numberOfIndices);

			for (size_t i = 0; i < numberOfIndices; ++i)
			{
				MeshIndex meshIndex = meshIndices[i];
				BottomLevelAccelerationStructureHandle handle = m_blasPerMesh[meshIndex];
				assert(handle != YAPT_NULL_HANDLE);
				blasHandlesToRelease.push_back(handle);
				m_blasPerMesh[meshIndex] = YAPT_NULL_HANDLE;
			}


			//update changed (Should probably check if the change actually affects the blas)
			mngr.getModifiedEntries(meshIndices, numberOfIndices);

			for (size_t i = 0; i < numberOfIndices; ++i)
			{
				MeshIndex meshIndex = meshIndices[i];

				//let go of old
				BottomLevelAccelerationStructureHandle handle = m_blasPerMesh[meshIndex];
				assert(handle != YAPT_NULL_HANDLE);
				blasHandlesToRelease.push_back(handle);
				m_blasPerMesh[meshIndex] = YAPT_NULL_HANDLE;

				const MeshInternal* mesh = mngr.getMeshInternal(meshIndex);
				geomDefs.resize(geomDefs.size() + 1);
				AccelerationStructureGeometryDefinition& def = geomDefs.back();
				fillGeometryDefinition(mesh, def);
				meshIndexOfBlas.push_back(meshIndex);
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

			for (size_t i = 0; i < blasHandles.size(); ++i)
			{
				size_t meshIndex = meshIndexOfBlas[i];
				m_blasPerMesh[meshIndex] = blasHandles[i];
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