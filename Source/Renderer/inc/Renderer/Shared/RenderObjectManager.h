#pragma once
#include <Common/TightlyPackedArray.h>
#include <Math/Math.h>
#include <Math/AABB.h>
#include <Gfx/GfxApi.h>
#include <Renderer/Shared/Utility/GpuBufferHelper.h>
#include <Common/RCObjectPtr.h>
#include <Common/JobSystem.h>
#include <Renderer/Shared/MaterialInternal.h>
#include <Renderer/Shared/MeshInternal.h>


namespace YAPT
{
	typedef TPAID RenderObjectId;
	constexpr RenderObjectId InvalidRenderObjectId = InvalidTPAID;
	constexpr size_t RENDEROBJECT_RENDERDATA_BUCKETS_COUNT = 6;
	constexpr size_t INVALID_RENDERDATA_INDEX = size_t(-1);

	class RenderObjectProxy;

	class CRenderer;
	class Material;

	class MeshManager;
	class MaterialManager;

	struct MaterialPerSubmeshArray
	{
		MaterialPerSubmeshArray()
		{}

		MaterialPerSubmeshArray(MaterialInternal** matPtr, size_t materialCount)
		{
			materials.resize(materialCount);
			for (size_t i = 0; i < materialCount; ++i)
			{
				materials[i] = matPtr[i]->getID();
			}
			
		}

		MaterialPerSubmeshArray(MaterialInternal* material)
		{
			materials.push_back(material->getID());
		}

		MaterialIndex getMaterialIDForSubmeshIndex(size_t submesh) const
		{
			if (submesh < materials.size())
			{
				return materials[submesh];
			}
			return materials.back();
		}

		std::vector<MaterialIndex> materials;
	};

	class RenderObjectManager
	{
		friend class RenderObjectProxy;
	public:

		union RenderData
		{
			size_t index;
			void* ptr;
		};

		RenderObjectManager(GfxApiHandle gfx, MeshManager& meshMngr, MaterialManager& matMngr);
		~RenderObjectManager();

		RenderObjectProxy* createRenderObject();
		void renderObjectReleased(RenderObjectProxy* obj);

		void commitChanges();

		void getCreatedEntries(const RenderObjectId*& ids, size_t& numberOfEntries) const
		{
			ids = m_createdEntries.data();
			numberOfEntries = m_createdEntries.size();
		}
		void getDestroyedEntries(const RenderObjectId*& ids, size_t& numberOfEntries) const
		{
			ids = m_destroyedEntries.data();
			numberOfEntries = m_destroyedEntries.size();
		}
		void getModifiedEntries(const RenderObjectId*& ids, size_t& numberOfEntries) const
		{
			ids = m_modifiedEntries.data();
			numberOfEntries = m_modifiedEntries.size();
		}

		mat4& getWorldMatrixForId(RenderObjectId id);
		mat4& getWorldInverseMatrixForId(RenderObjectId id);
		MeshIndex getMeshForId(RenderObjectId id);
		MaterialPerSubmeshArray& getMaterialForId(RenderObjectId id);
		AABB& getBoundsForId(RenderObjectId id);
		RenderData* getRenderDataForId(RenderObjectId id, size_t renderDataIndex);

		size_t getDataIndexForRenderObjectId(RenderObjectId id) const { return m_renderObjects.getDataIndex(id); }
		size_t getNumberOfObjects();
		mat4* getAllWorldMatrices();
		mat4* getAllWorldInverseMatrices();
		MeshIndex* getAllMeshes();
		MaterialPerSubmeshArray* getAllMaterials();
		AABB* getAllBounds();
		const RenderObjectId* getAllIds();
		RenderData* getAllRenderData(size_t renderDataIndex);
		RenderObjectId getHighestId() { return m_renderObjects.getHighestAllocatedId(); }

		bool hasChanges() const { return m_modifiedEntries.size() > 0 || m_destroyedEntries.size() > 0 || m_createdEntries.size() > 0; }

		size_t acquireRenderDataIndex();
		void freeRenderDataIndex(size_t index);
		constexpr bool isValidRenderDataIndex(size_t index) { return index != INVALID_RENDERDATA_INDEX && index < RENDEROBJECT_RENDERDATA_BUCKETS_COUNT; }

		JobHandle issueTransformAndBoundsUpdateJobs(JobSystem& pool);
		

	private:
		enum class RenderObjectPropertyIndex
		{
			Id,
			Transform,
			Transform_Inverse,
			Bounds,
			Mesh,
			Material,
			RenderData0,
			RenderData1,
			RenderData2,
			RenderData3,
			RenderData4,
			RenderData5
		};

		

		struct UpdateBoundsItem
		{
			MeshManager* meshMngr;
			size_t entryOffset;
			size_t entryCount;
			const MeshIndex* meshes;
			const mat4* transforms;
			mat4* transformsInv;
			AABB* bounds;
		};

		typedef TightlyPackedArray<RenderObjectId, mat4, mat4, AABB, MeshIndex, MaterialPerSubmeshArray, RenderData, RenderData, RenderData, RenderData, RenderData, RenderData> RenderObjectContainer;
		static const size_t PEROBJECTDATA_GROW_COUNT;

		void renderObjectChanged(RenderObjectProxy* obj);
		void addToChangedListIfNotAdded(RenderObjectProxy* obj);

		RenderObjectId createRenderObjectEntry(const RenderObjectProxy* proxy);
		void replicateChanges(RenderObjectProxy* src, size_t destinationIndex);
		void fillMaterials(const std::vector<RCObjectPtr<Material>>& src, MaterialPerSubmeshArray& dst);

		GfxApiHandle m_gfx;
		MeshManager& m_meshMngr;
		MaterialManager& m_matMngr;

		std::vector<RenderObjectId> m_createdEntries;
		std::vector<RenderObjectId> m_modifiedEntries;
		std::vector<RenderObjectId> m_destroyedEntries;

		RenderObjectContainer m_renderObjects;
		
		std::vector<UpdateBoundsItem> m_updateBoundsJobItems;

		std::vector<RenderObjectProxy*> m_changedRenderObjects;
		bool m_freeRenderDataIndices[RENDEROBJECT_RENDERDATA_BUCKETS_COUNT];
	};
}
