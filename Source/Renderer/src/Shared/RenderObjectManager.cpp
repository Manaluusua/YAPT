#include <Renderer/Shared/RenderObjectManager.h>
#include <Renderer/Shared/RenderObjectProxy.h>
#include <Renderer/Shared/MaterialProxy.h>
#include <Renderer/Shared/MeshProxy.h>
#include <Renderer/Shared/MeshInternal.h>
#include <Renderer/Shared/MaterialInternal.h>
#include <Common/CommonUtilities.h>
namespace YAPT
{
	const size_t RenderObjectManager::PEROBJECTDATA_GROW_COUNT = 256;

	RenderObjectManager::RenderObjectManager(GfxApiHandle gfx)
		:m_gfx(gfx),
		m_gpuData(gfx, RESOURCE_USAGE_COPY_DESTINATION | RESOURCE_USAGE_UNIFORM_BUFFER)
	{
		for (size_t i = 0; i < RENDEROBJECT_RENDERDATA_BUCKETS_COUNT; ++i)
		{
			m_freeRenderDataIndices[i] = true;
		}

	}
	RenderObjectManager::~RenderObjectManager()
	{
		
	}


	RenderObjectProxy* RenderObjectManager::createRenderObject()
	{
		RenderObjectProxy* ro = new RenderObjectProxy(this);
		ro->_renderObjectState = RenderObjectProxy::RENDEROBJECTSTATE_CREATED;
		m_changedRenderObjects.push_back(ro);
		return ro;
	}
	void RenderObjectManager::renderObjectReleased(RenderObjectProxy* obj)
	{
		addToChangedListIfNotAdded(obj);
		obj->_renderObjectState |= RenderObjectProxy::RENDEROBJECTSTATE_DESTROYED;
	}

	void RenderObjectManager::replicateChanges()
	{
		//let go of last frames released ids
		for (auto entry : m_destroyedEntries)
		{
			m_renderObjects.removeEntry(entry);
		}

		m_createdEntries.clear();
		m_modifiedEntries.clear();
		m_destroyedEntries.clear();

		for (size_t i = 0; i < m_changedRenderObjects.size(); ++i)
		{
			RenderObjectProxy* ro = m_changedRenderObjects[i];
			size_t state = ro->_renderObjectState;
			//check cornercase of being created and destroyed in the same frame
			if ((state & RenderObjectProxy::RENDEROBJECTSTATE_CREATED) != 0 && (state & RenderObjectProxy::RENDEROBJECTSTATE_DESTROYED) != 0)
			{
				delete ro;
				continue;
			}

			if ((state & RenderObjectProxy::RENDEROBJECTSTATE_CREATED) != 0)
			{
				ro->_id = createRenderObjectEntry(ro);
				m_createdEntries.push_back(ro->_id);
			}
			else if ((state & RenderObjectProxy::RENDEROBJECTSTATE_MODIFIED) != 0)
			{
				replicateChanges(ro, ro->_id);
				m_modifiedEntries.push_back(ro->_id);
			}



			if ((state & RenderObjectProxy::RENDEROBJECTSTATE_DESTROYED) != 0)
			{
				m_destroyedEntries.push_back(ro->_id);
				delete ro;
				ro = nullptr;
			}

			if (ro)
			{
				ro->_renderObjectState = RenderObjectProxy::RENDEROBJECTSTATE_NOCHANGES;
			}

		}

		m_changedRenderObjects.clear();
	}

	void RenderObjectManager::renderObjectChanged(RenderObjectProxy* obj)
	{
		addToChangedListIfNotAdded(obj);
		obj->_renderObjectState |= RenderObjectProxy::RENDEROBJECTSTATE_MODIFIED;
	}
	void RenderObjectManager::addToChangedListIfNotAdded(RenderObjectProxy* obj)
	{
		if (obj->_renderObjectState == RenderObjectProxy::RENDEROBJECTSTATE_NOCHANGES)
		{
			m_changedRenderObjects.push_back(obj);
		}
	}

	RenderObjectId RenderObjectManager::createRenderObjectEntry(const RenderObjectProxy* proxy)
	{
		mat4 t = proxy->m_transform;
		RenderData rData;
		rData.index = 0;
		//this is a bit funny but have to create the entry first with dummy id (0) and after creation replace it with correct id
		RenderObjectId id = m_renderObjects.addEntry(
			0, 
			t, 
			static_cast<MeshProxy*>(proxy->m_mesh.get())->getMeshInternal(), 
			static_cast<MaterialProxy*>(proxy->m_material.get())->getMaterialInternal(),
			rData, rData, rData, rData, rData, rData
		);
		m_renderObjects.getDataEntryWithId<(size_t)RenderObjectPropertyIndex::Id>(id) = id;
		return id;
	}
	void RenderObjectManager::replicateChanges(RenderObjectProxy* src, size_t dstIndex)
	{
		if ((src->_renderObjectState & RenderObjectProxy::RENDEROBJECTSTATE_MATERIAL_CHANGED) != 0)
		{
			m_renderObjects.getDataEntryWithId<(size_t)RenderObjectPropertyIndex::Material>(dstIndex) = static_cast<MaterialProxy*>(src->m_material.get())->getMaterialInternal();
		}

		if ((src->_renderObjectState & RenderObjectProxy::RENDEROBJECTSTATE_MESH_CHANGED) != 0)
		{
			m_renderObjects.getDataEntryWithId<(size_t)RenderObjectPropertyIndex::Mesh>(dstIndex) = static_cast<MeshProxy*>(src->m_mesh.get())->getMeshInternal();
		}

		if ((src->_renderObjectState & RenderObjectProxy::RENDEROBJECTSTATE_TRANSFORM_CHANGED) != 0)
		{
			m_renderObjects.getDataEntryWithId<(size_t)RenderObjectPropertyIndex::Transform>(dstIndex) = src->m_transform;
		}
	}

	mat4& RenderObjectManager::getMatrixForId(RenderObjectId id)
	{
		return m_renderObjects.getDataEntryWithId<(size_t)RenderObjectPropertyIndex::Transform>(id);
	}
	MeshInternal* RenderObjectManager::getMeshForId(RenderObjectId id)
	{
		return m_renderObjects.getDataEntryWithId<(size_t)RenderObjectPropertyIndex::Mesh>(id);
	}
	MaterialInternal* RenderObjectManager::getMaterialForId(RenderObjectId id)
	{
		return m_renderObjects.getDataEntryWithId<(size_t)RenderObjectPropertyIndex::Material>(id);
	}

	RenderObjectManager::RenderData* RenderObjectManager::getRenderDataForId(RenderObjectId id, size_t renderDataIndex)
	{
		assert(isValidRenderDataIndex(renderDataIndex));
		switch (renderDataIndex)
		{
		case 0:
			return &m_renderObjects.getDataEntryWithId<(size_t)RenderObjectPropertyIndex::RenderData0>(id);
			break;
		case 1:
			return &m_renderObjects.getDataEntryWithId<(size_t)RenderObjectPropertyIndex::RenderData1>(id);
			break;
		case 2:
			return &m_renderObjects.getDataEntryWithId<(size_t)RenderObjectPropertyIndex::RenderData2>(id);
			break;
		case 3:
			return &m_renderObjects.getDataEntryWithId<(size_t)RenderObjectPropertyIndex::RenderData3>(id);
			break;
		case 4:
			return &m_renderObjects.getDataEntryWithId<(size_t)RenderObjectPropertyIndex::RenderData4>(id);
			break;
		case 5:
			return &m_renderObjects.getDataEntryWithId<(size_t)RenderObjectPropertyIndex::RenderData5>(id);
			break;
		default:
			return nullptr;
		}
	}

	size_t RenderObjectManager::getNumberOfObjects()
	{
		return m_renderObjects.getDataCount();
	}
	mat4* RenderObjectManager::getAllMatrices()
	{
		return m_renderObjects.getData<(size_t)RenderObjectPropertyIndex::Transform>();
	}
	MeshInternal** RenderObjectManager::getAllMeshes()
	{
		return m_renderObjects.getData<(size_t)RenderObjectPropertyIndex::Mesh>();
	}
	MaterialInternal** RenderObjectManager::getAllMaterials()
	{
		return m_renderObjects.getData<(size_t)RenderObjectPropertyIndex::Material>();
	}

	const RenderObjectId* RenderObjectManager::getAllIds()
	{
		return m_renderObjects.getData<(size_t)RenderObjectPropertyIndex::Id>();
	}

	RenderObjectManager::RenderData* RenderObjectManager::getAllRenderData(size_t renderDataIndex)
	{
		assert(isValidRenderDataIndex(renderDataIndex));
		switch (renderDataIndex)
		{
		case 0:
			return m_renderObjects.getData<(size_t)RenderObjectPropertyIndex::RenderData0>();
			break;
		case 1:
			return m_renderObjects.getData<(size_t)RenderObjectPropertyIndex::RenderData1>();
			break;
		case 2:
			return m_renderObjects.getData<(size_t)RenderObjectPropertyIndex::RenderData2>();
			break;
		case 3:
			return m_renderObjects.getData<(size_t)RenderObjectPropertyIndex::RenderData3>();
			break;
		case 4:
			return m_renderObjects.getData<(size_t)RenderObjectPropertyIndex::RenderData4>();
			break;
		case 5:
			return m_renderObjects.getData<(size_t)RenderObjectPropertyIndex::RenderData5>();
			break;
		default:
			return nullptr;
		}
	}

	size_t RenderObjectManager::acquireRenderDataIndex()
	{
		size_t val = INVALID_RENDERDATA_INDEX;
		for (size_t i = 0; i < RENDEROBJECT_RENDERDATA_BUCKETS_COUNT; ++i)
		{
			if (m_freeRenderDataIndices[i])
			{
				m_freeRenderDataIndices[i] = false;
				val = i;
				break;
			}
		}
		assert(isValidRenderDataIndex(val));
		return val;
	}
	void RenderObjectManager::freeRenderDataIndex(size_t index)
	{
		assert(isValidRenderDataIndex(index));
		m_freeRenderDataIndices[index] = true;
	}



	void RenderObjectManager::updatePerObjectGPUData()
	{
		if (m_gpuData.getAllocatedEntryCount() < getNumberOfObjects())
		{
			size_t newEntries = align(getNumberOfObjects(), PEROBJECTDATA_GROW_COUNT);
			m_gpuData.allocate(newEntries);
		}
		char* data = m_gpuData.map(0, getNumberOfObjects());
		const mat4* matArray = getAllMatrices();
		for (size_t i = 0; i < getNumberOfObjects(); ++i)
		{
			memcpy(data + m_gpuData.getAlignedEntrySize() * i, matArray + i, sizeof(PerObjectGPUData));
		}
	}
}