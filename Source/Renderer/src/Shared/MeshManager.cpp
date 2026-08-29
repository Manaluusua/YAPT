#include <Renderer/Shared/MeshManager.h>
#include <Renderer/Shared/MeshProxy.h>

namespace YAPT
{
	MeshManager::MeshManager(GfxApiHandle gfx)
		:m_gfx(gfx),
		m_totalSubmeshCount(0),
		m_changedMeshes(256)
	{
		m_hashState = XXH64_createState();
	}
	MeshManager::~MeshManager()
	{
		XXH64_freeState(m_hashState);
	}
	 
	MeshProxy* MeshManager::createMesh(const VertexBufferLayout* layouts, size_t numberOfVertexBufferLayouts, size_t vertexCount, size_t submeshCount, bool use16BitIndices)
	{
		MeshProxy* mesh =  new MeshProxy(this, layouts, numberOfVertexBufferLayouts, vertexCount, submeshCount, use16BitIndices);
		mesh->_meshState = MeshProxy::MESHSTATE_INCOMPLETE;
		return mesh;
	}
	void MeshManager::meshReleased(MeshProxy* obj)
	{
		addToChangedListIfNotAdded(obj);
		obj->_meshState |= MeshProxy::MESHSTATE_DESTROYED;
	}
	 
	void MeshManager::commitChanges()
	{

		m_createdEntries.clear();
		m_modifiedEntries.clear();
		m_destroyedEntries.clear();

		MeshProxy** proxies = m_changedMeshes.getAll();

		for (size_t i = 0; i < m_changedMeshes.count(); ++i)
		{
			MeshProxy* meshImpl = proxies[i];
			size_t meshState = meshImpl->_meshState;
			//check cornercase of being created and destroyed in the same frame
			if (((meshState & MeshProxy::MESHSTATE_CREATED) != 0 || (meshState & MeshProxy::MESHSTATE_INCOMPLETE) != 0) && (meshState & MeshProxy::MESHSTATE_DESTROYED) != 0)
			{
				delete meshImpl;
				continue;
			}

			if ((meshState & MeshProxy::MESHSTATE_CREATED) != 0)
			{
				meshImpl->_id = createMeshEntry(meshImpl);
				MeshInternal& mesh = m_meshes.getDataEntryWithId(meshImpl->_id);
				replicateChanges(meshImpl, mesh);
				m_createdEntries.push_back(meshImpl->_id);
				m_totalSubmeshCount += meshImpl->getSubmeshCount();
			}
			else if ((meshState & MeshProxy::MESHSTATE_MODIFIED) != 0)
			{
				MeshInternal& mesh = m_meshes.getDataEntryWithId(meshImpl->_id);
				replicateChanges(meshImpl, mesh);
				m_modifiedEntries.push_back(meshImpl->_id);
			}

			if ((meshState & MeshProxy::MESHSTATE_DESTROYED) != 0)
			{
				m_destroyedEntries.push_back(meshImpl->_id);
				destroyMeshEntry(meshImpl);
				m_totalSubmeshCount -= meshImpl->getSubmeshCount();
				delete meshImpl;
				meshImpl = nullptr;
			}

			if (meshImpl)
			{
				meshImpl->_meshState = MeshProxy::MESHSTATE_NOCHANGES;
			}

		}

		for (auto entry : m_destroyedEntries)
		{
			m_meshes.removeEntry(entry);
		}

		m_changedMeshes.clear();
	}

	void MeshManager::meshChanged(MeshProxy* obj)
	{
		
		if ((obj->_meshState & MeshProxy::MESHSTATE_INCOMPLETE) != 0)
		{
			if (obj->hasAllRequiredBuffers())
			{
				*m_changedMeshes.add(1) = obj;
				obj->_meshState = MeshProxy::MESHSTATE_CREATED;
			}
		}
		else
		{
			addToChangedListIfNotAdded(obj);
			obj->_meshState |= MeshProxy::MESHSTATE_MODIFIED;
		}
		
	}


	void MeshManager::addToChangedListIfNotAdded(MeshProxy* obj)
	{
		if (obj->_meshState == MeshProxy::MESHSTATE_NOCHANGES)
		{
			*m_changedMeshes.add(1) = obj;
		}
	}

	MeshIndex MeshManager::createMeshEntry(MeshProxy* obj)
	{
		//real construction
		MeshLayoutID layoutID = getMeshLayoutIDForLayout(obj->getMeshLayoutInfo());
		MeshIndex id = m_meshes.addEntry(m_gfx, layoutID, obj->getMeshLayoutInfo(), obj->m_submeshes.size(), obj->has16BitIndices());
		return id;
	}

	MeshLayoutID MeshManager::getMeshLayoutIDForLayout(const MeshLayoutInfo& info)
	{
		
		XXH64_hash_t const seed = 0; 
		XXH64_reset(m_hashState, seed);

		for (size_t i = 0; i < info.vertexBufferConfigurations.size(); ++i)
		{
			const VertexBufferConfiguration& config = info.vertexBufferConfigurations[i];
			for (size_t k = 0; k < config.attributes.size(); ++k)
			{
				XXH64_update(m_hashState, config.attributes.data(), config.attributes.size() * sizeof(Attribute));
				XXH64_update(m_hashState, config.offsetFromVertexStart.data(), config.offsetFromVertexStart.size() * sizeof(uint32_t));
				XXH64_update(m_hashState, &config.stride, sizeof(uint32_t));
			}
		}

		XXH64_update(m_hashState, &info.numberOfVertices, sizeof(size_t));
		XXH64_hash_t const hash = XXH64_digest(m_hashState);
		

		return hash;
	}

	MeshInternal* MeshManager::getMeshInternal(MeshIndex id)
	{
		return &m_meshes.getDataEntryWithId(id);
	}
	

	void MeshManager::replicateChanges(MeshProxy* src, MeshInternal& dst)
	{
		for (size_t i = 0; i < src->m_vertexBuffers.size(); ++i)
		{
			dst.setVertexBuffer(i, src->m_vertexBuffers[i].buffer.get(), src->m_vertexBuffers[i].offsetInBytes);
		}

		dst.setIndexBuffer(src->m_indexBuffer.buffer.get(), src->m_indexBuffer.offsetInBytes);

		for (size_t i = 0; i < src->m_submeshes.size(); ++i)
		{
			dst.setSubmesh(i, src->m_submeshes[i]);
		}

		
	}

	void MeshManager::destroyMeshEntry(MeshProxy* obj)
	{
		//release resources
		MeshInternal& mesh = m_meshes.getDataEntryWithId(obj->_id);
		m_totalSubmeshCount -= mesh.getSubmeshCount();
		
	}
}