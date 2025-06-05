#include <Renderer/Shared/LightManager.h>
#include <Renderer/Shared/CRenderer.h>
#include <Common/ThreadPool.h>
namespace YAPT
{
	LightManager::LightManager(CRenderer* renderer)
		:m_renderer(renderer),
		m_lightReferencesCacheValid(false)
	{

	}
	LightManager::~LightManager()
	{

	}

	void LightManager::update(ThreadPool& pool)
	{
		RenderObjectManager& roMngr = m_renderer->getRenderObjectManager();
		MaterialManager& matMngr = m_renderer->getMaterialManager();

		if (!roMngr.hasChanges() && !matMngr.hasChanges())
		{
			return;
		}

		
		constexpr size_t MIN_ITEMS_PER_JOB = 100;
		size_t count = roMngr.getNumberOfObjects();
		size_t numberOfJobs = max(size_t(1), min(size_t(MAX_GATHER_JOBS), count / MIN_ITEMS_PER_JOB));
		size_t operationsPerJob = (count + numberOfJobs - 1) / numberOfJobs;

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
					const MaterialParameters& matParams = materialsPerSubmesh.getMaterialForSubmeshIndex(sm)->getMaterialParams();

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
			pool.addTask(gatherLightsFunc, &m_gatherLightsJobs[i]);
		}
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