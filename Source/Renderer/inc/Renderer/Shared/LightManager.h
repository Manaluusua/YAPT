#pragma once

#include <Renderer/Shared/RenderObjectManager.h>
#include <Renderer/Light.h>
#include <Common/GrowingMultiProducerPendingList.h>
#include <vector>
#include <array>

namespace YAPT
{
	class CRenderer;
	class ThreadPool;
	class LightProxy;

	struct PunctualLightParameters
	{
		LightType type;
		vec3p positionWS;
		vec3p directionWS; //emission direction, normalized
		vec3p intensity; //color * intensity
		float cosInnerAngle;
		float cosOuterAngle;
	};

	/**
		Keeps track of everything that emits light: emissive mesh submeshes (gathered from render objects each time objects or materials change)
		and explicitly created punctual lights.
	*/
	class LightManager
	{
		friend class LightProxy;
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

		LightProxy* createLight(LightType type);
		void lightReleased(LightProxy* light);

		void commitChanges();

		JobHandle update(JobSystem& pool, JobHandle deps);

		const LightReference* getAllLightReferences() const { checkLightCacheValid();  return m_lightReferencesCache.data(); }
		size_t getLightReferenceCount() const { checkLightCacheValid(); return m_lightReferencesCache.size(); }

		const PunctualLightParameters* getAllPunctualLights() const { return m_punctualLightsPacked.data(); }
		size_t getPunctualLightCount() const { return m_punctualLightsPacked.size(); }

		bool hasPunctualLightChanges() const { return m_punctualLightsChanged; }

	private:

		static constexpr size_t MAX_GATHER_JOBS = 8;

		struct GatherLightsJob
		{
			MaterialManager* matMngr;
			MaterialPerSubmeshArray* materials;
			const RenderObjectId* ids;
			size_t offset;
			size_t count;

			std::vector<LightReference> referencesFound;
		};

		void lightChanged(LightProxy* light);
		void addToChangedListIfNotAdded(LightProxy* light);
		static void fillParameters(const LightProxy* src, PunctualLightParameters& dst);

		void checkLightCacheValid() const;

		CRenderer* m_renderer;
		std::array<GatherLightsJob, MAX_GATHER_JOBS> m_gatherLightsJobs;

		mutable std::vector<LightReference> m_lightReferencesCache;
		mutable bool m_lightReferencesCacheValid;

		std::vector<LightProxy*> m_punctualLights; //proxy _id is the index in this array
		std::vector<PunctualLightParameters> m_punctualLightsPacked;
		GrowingMultiProducerPendingList<LightProxy*> m_changedLights;
		bool m_punctualLightsChanged;
	};
}

