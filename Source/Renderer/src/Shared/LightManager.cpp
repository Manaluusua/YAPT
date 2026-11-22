#include <Renderer/Shared/LightManager.h>
#include <Renderer/Shared/CRenderer.h>
#include <Renderer/Shared/MaterialManager.h>
#include <Renderer/Shared/LightProxy.h>
#include <Common/JobSystem.h>
namespace YAPT
{
	LightManager::LightManager(CRenderer* renderer)
		:m_renderer(renderer),
		m_lightReferencesCacheValid(false),
		m_changedLights(64),
		m_punctualLightsChanged(false)
	{

	}
	LightManager::~LightManager()
	{

	}

	LightProxy* LightManager::createLight(LightType type)
	{
		LightProxy* light = new LightProxy(this, type);
		light->_lightState = LightProxy::LIGHTSTATE_CREATED;
		*m_changedLights.add(1) = light;
		return light;
	}

	void LightManager::lightReleased(LightProxy* light)
	{
		addToChangedListIfNotAdded(light);
		light->_lightState |= LightProxy::LIGHTSTATE_DESTROYED;
	}

	void LightManager::lightChanged(LightProxy* light)
	{
		addToChangedListIfNotAdded(light);
		light->_lightState |= LightProxy::LIGHTSTATE_MODIFIED;
	}

	void LightManager::addToChangedListIfNotAdded(LightProxy* light)
	{
		if (light->_lightState == LightProxy::LIGHTSTATE_NOCHANGES)
		{
			*m_changedLights.add(1) = light;
		}
	}

	void LightManager::commitChanges()
	{
		m_punctualLightsChanged = m_changedLights.count() > 0;

		LightProxy** proxies = m_changedLights.getAll();
		for (size_t i = 0; i < m_changedLights.count(); ++i)
		{
			LightProxy* light = proxies[i];
			size_t state = light->_lightState;
			bool created = (state & LightProxy::LIGHTSTATE_CREATED) != 0;
			bool destroyed = (state & LightProxy::LIGHTSTATE_DESTROYED) != 0;

			if (created && destroyed)
			{
				delete light;
				continue;
			}

			if (created)
			{
				light->_id = m_punctualLights.size();
				m_punctualLights.push_back(light);
			}

			if (destroyed)
			{
				size_t index = light->_id;
				m_punctualLights[index] = m_punctualLights.back();
				m_punctualLights[index]->_id = index;
				m_punctualLights.pop_back();
				delete light;
				continue;
			}

			light->_lightState = LightProxy::LIGHTSTATE_NOCHANGES;
		}
		m_changedLights.clear();

		if (m_punctualLightsChanged)
		{
			m_punctualLightsPacked.resize(m_punctualLights.size());
			for (size_t i = 0; i < m_punctualLights.size(); ++i)
			{
				fillParameters(m_punctualLights[i], m_punctualLightsPacked[i]);
			}
		}
	}

	void LightManager::fillParameters(const LightProxy* src, PunctualLightParameters& dst)
	{
		dst.type = src->m_type;
		dst.positionWS = vec3p(src->m_transform[3]);
		vec3 dir = mat3(src->m_transform) * s_worldForward;
		float dirLen = glm::length(dir);
		dst.directionWS = dirLen > 0.f ? dir / dirLen : s_worldForward;
		dst.intensity = src->m_color * glm::max(0.f, src->m_intensity);
		dst.cosInnerAngle = glm::cos(src->m_spotInnerAngle);
		dst.cosOuterAngle = glm::cos(src->m_spotOuterAngle);
	}

	JobHandle LightManager::update(JobSystem& pool, JobHandle deps)
	{
		RenderObjectManager& roMngr = m_renderer->getRenderObjectManager();
		MaterialManager& matMngr = m_renderer->getMaterialManager();

		if (!roMngr.hasChanges() && !matMngr.hasChanges())
		{
			return {};
		}

		
		constexpr size_t MIN_ITEMS_PER_JOB = 100;
		size_t count = roMngr.getNumberOfObjects();
		size_t numberOfJobs = max(size_t(1), min(size_t(MAX_GATHER_JOBS), count / MIN_ITEMS_PER_JOB));
		size_t operationsPerJob = (count + numberOfJobs - 1) / numberOfJobs;

		std::array<JobHandle, MAX_GATHER_JOBS> lightJobs;

		m_lightReferencesCache.clear();
		m_lightReferencesCacheValid = false;

		auto gatherLightsFunc = [](void* usrData)
		{
			GatherLightsJob* item = static_cast<GatherLightsJob*>(usrData);
			item->referencesFound.clear();
			for (size_t i = 0; i < item->count; ++i)
			{
				size_t index = item->offset + i;
				const MaterialPerSubmeshArray& materialsPerSubmesh = item->materials[index];


				for (size_t sm = 0; sm < materialsPerSubmesh.materials.size(); ++sm)
				{
					MaterialIndex matID = materialsPerSubmesh.getMaterialIDForSubmeshIndex(sm);
					const MaterialParameters& matParams = item->matMngr->getMaterialInternal(matID)->getMaterialParams();

					if ((glm::dot(matParams.emissive, vec3p(1, 1, 1)) > 0) || matParams.emissiveTex.textureIndex != TEX_UNBOUND_INDEX)
					{
						RenderObjectId id = item->ids[index];
						item->referencesFound.emplace_back(id, sm);
					}
				}

			}
		};

		MaterialPerSubmeshArray* allMaterialReferences = roMngr.getAllMaterials();
		const RenderObjectId* ids = roMngr.getAllIds();

		size_t offset = 0;
		for (size_t i = 0; i < numberOfJobs; ++i)
		{
			GatherLightsJob& item = m_gatherLightsJobs[i];
			item.matMngr = &m_renderer->getMaterialManager();
			item.materials = allMaterialReferences;
			item.ids = ids;
			item.count = min(operationsPerJob, count - offset);
			item.offset = offset;
			offset += item.count;

		}

		for (size_t i = numberOfJobs; i < MAX_GATHER_JOBS; ++i)
		{
			m_gatherLightsJobs[i].referencesFound.clear();
		}


		for (size_t i = 0; i < numberOfJobs; ++i)
		{
			lightJobs[i] = pool.submit(gatherLightsFunc, &m_gatherLightsJobs[i], &deps, 1);
		}

		return pool.combineDependencies(lightJobs.data(), numberOfJobs);
	}

	void LightManager::checkLightCacheValid() const
	{
		if (m_lightReferencesCacheValid) return;

		size_t lightCount = 0;
		for (size_t i = 0; i < MAX_GATHER_JOBS; ++i)
		{
			lightCount += m_gatherLightsJobs[i].referencesFound.size();
		}

		m_lightReferencesCache.reserve(lightCount);

	
		for (size_t i = 0; i < MAX_GATHER_JOBS; ++i)
		{
			const std::vector<LightReference>& references = m_gatherLightsJobs[i].referencesFound;
			for (size_t k = 0; k < references.size(); ++k)
			{
				m_lightReferencesCache.push_back(references[k]);
			}
		}

		m_lightReferencesCacheValid = true;
	}
}