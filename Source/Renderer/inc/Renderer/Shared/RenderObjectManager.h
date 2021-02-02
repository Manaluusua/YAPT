#pragma once


#include <Common/TightlyPackedArray.h>
#include <Math/Math.h>
#include <Renderer/Shared/GfxApi.h>
#include <Renderer/Shared/Utility/GpuBufferHelper.h>

namespace YAPT
{
	typedef TPAID RenderObjectId;
	constexpr RenderObjectId InvalidRenderObjectId = InvalidTPAID;
	constexpr size_t RENDEROBJECT_RENDERDATA_BUCKETS_COUNT = 6;
	constexpr size_t INVALID_RENDERDATA_INDEX = size_t(-1);

	class RenderObjectProxy;

	class MaterialInternal;
	class MeshInternal;
	class CRenderer;

	class RenderObjectManager
	{
		friend class RenderObjectProxy;
	public:

		union RenderData
		{

			size_t index;
			void* ptr;

		};

		RenderObjectManager(GfxApiHandle gfx);
		~RenderObjectManager();

		RenderObjectProxy* createRenderObject();
		void renderObjectReleased(RenderObjectProxy* obj);

		void replicateChanges();
		void updatePerObjectGPUData();

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

		mat4& getMatrixForId(RenderObjectId id);
		MeshInternal* getMeshForId(RenderObjectId id);
		MaterialInternal* getMaterialForId(RenderObjectId id);
		RenderData* getRenderDataForId(RenderObjectId id, size_t renderDataIndex);

		size_t getDataIndexForRenderObjectId(RenderObjectId id) const { return m_renderObjects.getDataIndex(id); }
		size_t getNumberOfObjects();
		mat4* getAllMatrices();
		MeshInternal** getAllMeshes();
		MaterialInternal** getAllMaterials();
		const RenderObjectId* getAllIds();
		RenderData* getAllRenderData(size_t renderDataIndex);
		RenderObjectId getHighestId() { return m_renderObjects.getHighestAllocatedId(); }

		bool hasChanges() const { return m_modifiedEntries.size() > 0 || m_destroyedEntries.size() > 0 || m_createdEntries.size() > 0; }

		size_t acquireRenderDataIndex();
		void freeRenderDataIndex(size_t index);
		constexpr bool isValidRenderDataIndex(size_t index) { return index != INVALID_RENDERDATA_INDEX && index < RENDEROBJECT_RENDERDATA_BUCKETS_COUNT; }

	private:
		enum class RenderObjectPropertyIndex
		{
			Id,
			Transform,
			Mesh,
			Material,
			RenderData0,
			RenderData1,
			RenderData2,
			RenderData3,
			RenderData4,
			RenderData5
		};

		struct PerObjectGPUData
		{
			mat4 worldMatrix;
		};

		typedef TightlyPackedArray<RenderObjectId, mat4, MeshInternal*, MaterialInternal*, RenderData, RenderData, RenderData, RenderData, RenderData, RenderData> RenderObjectContainer;
		static const size_t PEROBJECTDATA_GROW_COUNT;

		void renderObjectChanged(RenderObjectProxy* obj);
		void addToChangedListIfNotAdded(RenderObjectProxy* obj);

		RenderObjectId createRenderObjectEntry(const RenderObjectProxy* proxy);
		void replicateChanges(RenderObjectProxy* src, size_t destinationIndex);

		void allocateGPUBuffer(size_t entryCount);

		GfxApiHandle m_gfx;

		std::vector<RenderObjectId> m_createdEntries;
		std::vector<RenderObjectId> m_modifiedEntries;
		std::vector<RenderObjectId> m_destroyedEntries;

		RenderObjectContainer m_renderObjects;

		DynamicSizeGpuBufferHelper< PerObjectGPUData> m_gpuData;
		

		std::vector<RenderObjectProxy*> m_changedRenderObjects;
		bool m_freeRenderDataIndices[RENDEROBJECT_RENDERDATA_BUCKETS_COUNT];
	};
}
