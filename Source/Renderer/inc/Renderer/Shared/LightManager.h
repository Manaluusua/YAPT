#pragma once

#include <Renderer/Shared/RenderObjectManager.h>
#include <vector>
#include <array>

namespace YAPT
{
	class CRenderer;
	class ThreadPool;
	class LightManager
	{
	public:

		struct LightReference
		{
			LightReference() = default;
			LightReference(RenderObjectId id, size_t sm)
				:objectId(id),
				submeshIndex(sm)
			{

			}
			RenderObjectId objectId;
			size_t submeshIndex;
		};

		LightManager(CRenderer* renderer);
		~LightManager();

		void update(ThreadPool& pool);

		const LightReference* getAllLightReferences() const { checkLightCacheValid();  return m_lightReferencesCache.data(); }
		size_t getLightReferenceCount() const { checkLightCacheValid(); return m_lightReferencesCache.size(); }

	private:

		static constexpr size_t MAX_GATHER_JOBS = 4;

		struct GatherLightsJob
		{
			MaterialManager* matMngr;
			MaterialPerSubmeshArray* materials;
			const RenderObjectId* ids;
			size_t offset;
			size_t count;

			std::vector<LightReference> referencesFound;
		};

		void checkLightCacheValid() const;

		CRenderer* m_renderer;
		std::array<GatherLightsJob, MAX_GATHER_JOBS> m_gatherLightsJobs;

		mutable std::vector<LightReference> m_lightReferencesCache;
		mutable bool m_lightReferencesCacheValid;
	};
}

