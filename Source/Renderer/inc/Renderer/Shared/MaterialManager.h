#ifndef YAPT_SHARED_MATERIALMANAGER_H
#define YAPT_SHARED_MATERIALMANAGER_H

#include <Renderer/Material.h>
#include <Common/BubbleArray.h>
#include <Renderer/Shared/MaterialInternal.h>
#include <Common/GrowingMultiProducerPendingList.h>
namespace YAPT
{
	

	class MaterialProxy;
	class MaterialManager
	{
		friend class MaterialProxy;
	public:
		typedef BubbleArray<MaterialInternal>::Iterator MaterialIterator;
		typedef BubbleArray<MaterialInternal>::ConstIterator ConstMaterialIterator;

		MaterialManager();
		~MaterialManager();

		MaterialProxy* createMaterial();
		void materialReleased(MaterialProxy* obj);

		void replicateChanges();

		MaterialInternal* getMaterialInternal(MaterialIndex id);

		void getCreatedEntries(const MaterialIndex*& ids, size_t& numberOfEntries) const
		{
			ids = m_createdEntries.data();
			numberOfEntries = m_createdEntries.size();
		}
		void getDestroyedEntries(const MaterialIndex*& ids, size_t& numberOfEntries) const
		{
			ids = m_destroyedEntries.data();
			numberOfEntries = m_destroyedEntries.size();
		}
		void getModifiedEntries(const MaterialIndex*& ids, size_t& numberOfEntries) const
		{
			ids = m_modifiedEntries.data();
			numberOfEntries = m_modifiedEntries.size();
		}

		MaterialIterator getMaterialIterator() { return m_materials.getIterator(); }
		ConstMaterialIterator getMaterialIterator() const { return m_materials.getConstIterator(); }
		size_t getHighestAllocatedIndex() const { return m_materials.getHighestIndexAllocated(); }
		size_t getActiveEntriesCount() const { return m_materials.getNumberOfActiveEntries(); }

		bool hasChanges() const { return m_modifiedEntries.size() > 0 || m_destroyedEntries.size() > 0 || m_createdEntries.size() > 0; }

	private:
		void materialChanged(MaterialProxy* obj);
		void addToChangedListIfNotAdded(MaterialProxy* obj);

		MaterialIndex createMaterialEntry(const MaterialProxy* proxy);
		void replicateChanges(const MaterialProxy* src, MaterialInternal& dst);

		std::vector<MaterialIndex> m_createdEntries;
		std::vector<MaterialIndex> m_modifiedEntries;
		std::vector<MaterialIndex> m_destroyedEntries;

		BubbleArray<MaterialInternal> m_materials;

		GrowingMultiProducerPendingList<MaterialProxy*> m_changedMaterials;

	};
}

#endif