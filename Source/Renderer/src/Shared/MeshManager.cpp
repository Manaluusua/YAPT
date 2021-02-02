#include <Renderer/Shared/MeshManager.h>
#include <Renderer/Shared/MeshProxy.h>
namespace YAPT
{
	MeshManager::MeshManager(GfxApiHandle gfx)
		:m_gfx(gfx)
	{

	}
	MeshManager::~MeshManager()
	{

	}
	 
	MeshProxy* MeshManager::createMesh(const VertexBufferLayout* layouts, size_t numberOfVertexBufferLayouts, size_t vertexCount)
	{
		MeshProxy* mesh =  new MeshProxy(this, layouts, numberOfVertexBufferLayouts, vertexCount);
		mesh->_meshState = MeshProxy::MESHSTATE_CREATED;
		m_changedMeshes.push_back(mesh);
		return mesh;
	}
	void MeshManager::meshReleased(MeshProxy* obj)
	{
		addToChangedListIfNotAdded(obj);
		obj->_meshState |= MeshProxy::MESHSTATE_DESTROYED;
	}
	 
	void MeshManager::replicateChanges()
	{

		//let go of last frames released ids
		for (auto entry : m_destroyedEntries)
		{
			m_meshes.removeEntry(entry);
		}
		m_createdEntries.clear();
		m_modifiedEntries.clear();
		m_destroyedEntries.clear();

		for (size_t i = 0; i < m_changedMeshes.size(); ++i)
		{
			MeshProxy* meshImpl = m_changedMeshes[i];
			size_t meshState = meshImpl->_meshState;
			//check cornercase of being created and destroyed in the same frame
			if ((meshState & MeshProxy::MESHSTATE_CREATED) != 0 && (meshState & MeshProxy::MESHSTATE_DESTROYED) != 0)
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
				delete meshImpl;
				meshImpl = nullptr;
			}

			if (meshImpl)
			{
				meshImpl->_meshState = MeshProxy::MESHSTATE_NOCHANGES;
			}

		}

		m_changedMeshes.clear();
	}

	void MeshManager::meshChanged(MeshProxy* obj)
	{
		addToChangedListIfNotAdded(obj);
		obj->_meshState |= MeshProxy::MESHSTATE_MODIFIED;
	}


	void MeshManager::addToChangedListIfNotAdded(MeshProxy* obj)
	{
		if (obj->_meshState == MeshProxy::MESHSTATE_NOCHANGES)
		{
			m_changedMeshes.push_back(obj);
		}
	}

	MeshIndex MeshManager::createMeshEntry(MeshProxy* obj)
	{
		//real construction
		MeshLayoutID layoutID = getMeshLayoutIDForLayout(obj->getMeshLayoutInfo());
		MeshIndex id = m_meshes.addEntry(m_gfx, layoutID, obj->getMeshLayoutInfo());
		return id;
	}

	MeshLayoutID MeshManager::getMeshLayoutIDForLayout(const MeshLayoutInfo& info)
	{
		static size_t id = 0;
		return id++; //TODO: implement properly
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
		dst.setPrimitiveCount(src->m_primitiveCount);
	}

	void MeshManager::destroyMeshEntry(MeshProxy* obj)
	{
		//release resources
		MeshInternal& mesh = m_meshes.getDataEntryWithId(obj->_id);
		
	}
}