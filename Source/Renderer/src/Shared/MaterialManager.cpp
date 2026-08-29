#include <Renderer/Shared/MaterialManager.h>
#include <Renderer/Shared/MaterialProxy.h>
namespace YAPT
{
	MaterialManager::MaterialManager()
		:m_changedMaterials(256)
	{

	}
	MaterialManager::~MaterialManager()
	{

	}

	MaterialProxy* MaterialManager::createMaterial()
	{
		MaterialProxy* mat = new MaterialProxy(this);
		mat->_materialState = MaterialProxy::MATERIALSTATE_CREATED;
		*m_changedMaterials.add(1) = mat;
		return mat;
	}
	void MaterialManager::materialReleased(MaterialProxy* obj)
	{
		addToChangedListIfNotAdded(obj);
		obj->_materialState |= MaterialProxy::MATERIALSTATE_DESTROYED;
	}

	void MaterialManager::commitChanges()
	{
		

		m_createdEntries.clear();
		m_modifiedEntries.clear();
		m_destroyedEntries.clear();
		MaterialProxy** proxies = m_changedMaterials.getAll();
		for (size_t i = 0; i < m_changedMaterials.count(); ++i)
		{
			MaterialProxy* mat = proxies[i];
			size_t matState = mat->_materialState;
			//check cornercase of being created and destroyed in the same frame
			if ((matState & MaterialProxy::MATERIALSTATE_CREATED) != 0 && (matState & MaterialProxy::MATERIALSTATE_DESTROYED) != 0)
			{
				delete mat;
				continue;
			}

			if ((matState & MaterialProxy::MATERIALSTATE_CREATED) != 0)
			{
				mat->_id = createMaterialEntry(mat);
				m_createdEntries.push_back(mat->_id);
			}
			else if ((matState & MaterialProxy::MATERIALSTATE_MODIFIED) != 0)
			{
				MaterialInternal& matEntry = m_materials.getDataEntryWithId(mat->_id);
				replicateChanges(mat, matEntry);
				m_modifiedEntries.push_back(mat->_id);
			}

			if ((matState & MaterialProxy::MATERIALSTATE_DESTROYED) != 0)
			{
				m_destroyedEntries.push_back(mat->_id);
				delete mat;
				mat = nullptr;
			}

			if (mat)
			{
				mat->_materialState = MaterialProxy::MATERIALSTATE_NOCHANGES;
			}

		}

		for (auto entry : m_destroyedEntries)
		{
			m_materials.removeEntry(entry);
		}

		m_changedMaterials.clear();
	}



	MaterialInternal* MaterialManager::getMaterialInternal(MaterialIndex id)
	{
		return &m_materials.getDataEntryWithId(id);
	}

	void MaterialManager::materialChanged(MaterialProxy* obj)
	{
		addToChangedListIfNotAdded(obj);
		obj->_materialState |= MaterialProxy::MATERIALSTATE_MODIFIED;
	}

	void MaterialManager::replicateChanges(const MaterialProxy* src, MaterialInternal& dst)
	{
		dst.setMaterialParams(src->m_materialParams);
	}

	void MaterialManager::addToChangedListIfNotAdded(MaterialProxy* obj)
	{
		if (obj->_materialState == MaterialProxy::MATERIALSTATE_NOCHANGES)
		{
			*m_changedMaterials.add(1) = obj;
		}
	}

	MaterialIndex MaterialManager::createMaterialEntry(const MaterialProxy* proxy)
	{
		MaterialIndex matIndex = m_materials.addEntry();
		replicateChanges(proxy, m_materials.getDataEntryWithId(matIndex));

		return matIndex;
	}
	
}