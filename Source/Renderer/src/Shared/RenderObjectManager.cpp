#include <Renderer/Shared/RenderObjectManager.h>
#include <Renderer/Shared/RenderObjectProxy.h>
#include <Renderer/Shared/MaterialProxy.h>
#include <Renderer/Shared/MeshProxy.h>
#include <Renderer/Shared/MeshInternal.h>
#include <Renderer/Shared/MaterialInternal.h>
#include <Renderer/Shared/MeshManager.h>
#include <Common/CommonUtilities.h>
#include <Common/ThreadPool.h>

namespace YAPT
{
	const size_t RenderObjectManager::PEROBJECTDATA_GROW_COUNT = 256;

	RenderObjectManager::RenderObjectManager(GfxApiHandle gfx, MeshManager& meshMngr, MaterialManager& matMngr)
		:m_gfx(gfx),
		m_meshMngr(meshMngr),
		m_matMngr(matMngr),
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

	void RenderObjectManager::fillMaterials(const std::vector<RCObjectPtr<Material>>& src, MaterialPerSubmeshArray& dst)
	{
		dst.materials.resize(src.size());

		for (size_t i = 0; i < src.size(); ++i)
		{
			dst.materials[i] = static_cast<MaterialProxy*>(src[i].get())->getMaterialInternal()->getID();
		}
	}

	RenderObjectId RenderObjectManager::createRenderObjectEntry(const RenderObjectProxy* proxy)
	{
		mat4 t = proxy->m_transform;
		RenderData rData;
		rData.index = 0;

		MaterialPerSubmeshArray matArray;
		fillMaterials(proxy->m_materials, matArray);

		//this is a bit funny but have to create the entry first with dummy id (0) and after creation replace it with correct id
		RenderObjectId id = m_renderObjects.addEntry(
			0, 
			t, 
			AABB::createEmpty(),
			static_cast<MeshProxy*>(proxy->m_mesh.get())->getMeshInternal()->getID(),
			matArray,
			rData, rData, rData, rData, rData, rData
		);
		m_renderObjects.getDataEntryWithId<(size_t)RenderObjectPropertyIndex::Id>(id) = id;
		return id;
	}
	void RenderObjectManager::replicateChanges(RenderObjectProxy* src, size_t dstIndex)
	{
		if ((src->_renderObjectState & RenderObjectProxy::RENDEROBJECTSTATE_MATERIAL_CHANGED) != 0)
		{
			MaterialPerSubmeshArray matArray;
			fillMaterials(src->m_materials, matArray);

			m_renderObjects.getDataEntryWithId<(size_t)RenderObjectPropertyIndex::Material>(dstIndex) = matArray;
		}

		if ((src->_renderObjectState & RenderObjectProxy::RENDEROBJECTSTATE_MESH_CHANGED) != 0)
		{
			m_renderObjects.getDataEntryWithId<(size_t)RenderObjectPropertyIndex::Mesh>(dstIndex) = static_cast<MeshProxy*>(src->m_mesh.get())->getMeshInternal()->getID();
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
	MeshIndex RenderObjectManager::getMeshForId(RenderObjectId id)
	{
		return m_renderObjects.getDataEntryWithId<(size_t)RenderObjectPropertyIndex::Mesh>(id);
	}
	MaterialPerSubmeshArray& RenderObjectManager::getMaterialForId(RenderObjectId id)
	{
		return m_renderObjects.getDataEntryWithId<(size_t)RenderObjectPropertyIndex::Material>(id);
	}

	AABB& RenderObjectManager::getBoundsForId(RenderObjectId id)
	{
		return m_renderObjects.getDataEntryWithId<(size_t)RenderObjectPropertyIndex::Bounds>(id);
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
	MeshIndex* RenderObjectManager::getAllMeshes()
	{
		return m_renderObjects.getData<(size_t)RenderObjectPropertyIndex::Mesh>();
	}
	MaterialPerSubmeshArray* RenderObjectManager::getAllMaterials()
	{
		return m_renderObjects.getData<(size_t)RenderObjectPropertyIndex::Material>();
	}

	AABB* RenderObjectManager::getAllBounds()
	{
		return m_renderObjects.getData<(size_t)RenderObjectPropertyIndex::Bounds>();
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

	void RenderObjectManager::issueBoundsUpdateJobs(ThreadPool& pool)
	{
		constexpr size_t MAX_JOBS = 16;
		constexpr size_t MIN_ITEMS_PER_JOB = 100;
		YAPT::mat4* wMat = getAllMatrices();
		MeshIndex* meshes = getAllMeshes();
		AABB* bounds = getAllBounds();
		size_t count = getNumberOfObjects();
		size_t numberOfJobs = max(size_t(1), min(size_t(MAX_JOBS), count / MIN_ITEMS_PER_JOB));
		size_t operationsPerJob = (count + numberOfJobs - 1) / numberOfJobs;

		if (count == 0)
		{
			return;
		}

		m_updateBoundsJobItems.resize(numberOfJobs);

		auto updateBoundsFunc = [](void* usrData)
		{
			UpdateBoundsItem* item = static_cast<UpdateBoundsItem*>(usrData);
			for (size_t i = 0; i < item->entryCount; ++i)
			{
				size_t index = item->entryOffset + i;
				MeshIndex meshID = item->meshes[index];
				MeshInternal* mesh = item->meshMngr->getMeshInternal(meshID);
				const mat4& t = *item->transforms;

				AABB meshBounds = AABB::createEmpty();

				for (size_t sm = 0; sm < mesh->getSubmeshCount(); ++sm)
				{
					const AABB& submeshBounds = mesh->getSubmesh(sm).bounds;
					meshBounds.encapsulate(submeshBounds);
				}

				AABB transformedBounds = AABB::createEmpty();

				for (size_t c = 0; c < 8; ++c)
				{
					float x = (c % 2) == 0 ? meshBounds.min.x : meshBounds.max.x;
					float y = ((c / 2) % 2) == 0 ? meshBounds.min.y : meshBounds.max.y;
					float z = c < 4 ? meshBounds.min.z : meshBounds.max.z;

					transformedBounds.encapsulate(t * glm::vec4(x, y, z, 1.f));
				}

				item->bounds[index] = transformedBounds;
			}



		};

		size_t offset = 0;
		for (size_t i = 0; i < numberOfJobs; ++i)
		{
			UpdateBoundsItem& item = m_updateBoundsJobItems[i];
			item.meshMngr = &m_meshMngr;
			item.bounds = bounds;
			item.meshes = meshes;
			item.transforms = wMat;
			item.entryCount = min(operationsPerJob, count - offset);
			offset += item.entryCount;

		}


		for (size_t i = 0; i < numberOfJobs; ++i)
		{
			pool.addTask(updateBoundsFunc, &m_updateBoundsJobItems[i]);
		}
	}


	void RenderObjectManager::updatePerObjectGPUData()
	{
		if (getNumberOfObjects() == 0) return;
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