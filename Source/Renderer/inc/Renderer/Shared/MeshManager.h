#pragma once

#include <Renderer/Mesh.h>
#include <Common/BubbleArray.h>
#include <Renderer/Buffer.h>
#include <Common/RCObjectPtr.h>
#include <Renderer/Shared/MeshInternal.h>
#include <Renderer/Shared/GfxApi.h>

namespace YAPT
{
	typedef size_t MeshIndex;
	constexpr MeshIndex InvalidMeshIndex = InvalidBubbleArrayIndex;

	class MeshProxy;
	class MeshManager
	{
		friend class MeshProxy;
	public:
		typedef BubbleArray<MeshInternal>::Iterator MeshIterator;
		typedef BubbleArray<MeshInternal>::ConstIterator ConstMeshIterator;
		
		MeshManager(GfxApiHandle gfx);
		~MeshManager();

		MeshProxy* createMesh(const VertexBufferLayout* layouts, size_t numberOfVertexBufferLayouts, size_t vertexCount);
		void meshReleased(MeshProxy* obj);

		void replicateChanges();

		MeshInternal* getMeshInternal(MeshIndex id);

		void getCreatedEntries(const MeshIndex*& ids, size_t& numberOfEntries) const
		{
			ids = m_createdEntries.data();
			numberOfEntries = m_createdEntries.size();
		}
		void getDestroyedEntries(const MeshIndex*& ids, size_t& numberOfEntries) const
		{
			ids = m_createdEntries.data();
			numberOfEntries = m_createdEntries.size();
		}
		void getModifiedEntries(const MeshIndex*& ids, size_t& numberOfEntries) const
		{
			ids = m_createdEntries.data();
			numberOfEntries = m_createdEntries.size();
		}

		MeshIterator getMeshIterator() { return m_meshes.getIterator(); }
		ConstMeshIterator getMeshIterator() const { return m_meshes.getConstIterator(); }
		size_t getHighestAllocatedIndex() const { return m_meshes.getHighestIndexAllocated(); }

		bool hasChanges() const { return m_modifiedEntries.size() > 0 || m_destroyedEntries.size() > 0 || m_createdEntries.size() > 0; }

	private:
		void meshChanged(MeshProxy* obj);
		void addToChangedListIfNotAdded(MeshProxy* obj);
		
		MeshLayoutID getMeshLayoutIDForLayout(const MeshLayoutInfo& id);

		MeshIndex createMeshEntry(MeshProxy* obj);
		void replicateChanges(MeshProxy* src, MeshInternal& dst);
		void destroyMeshEntry(MeshProxy* obj);

		GfxApiHandle m_gfx;

		std::vector<MeshIndex> m_createdEntries;
		std::vector<MeshIndex> m_modifiedEntries;
		std::vector<MeshIndex> m_destroyedEntries;

		BubbleArray<MeshInternal> m_meshes;

		std::vector<MeshProxy*> m_changedMeshes;

	};
}
