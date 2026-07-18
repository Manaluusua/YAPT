#include <Renderer/Shared/Utility/MultipleScatterLUT.h>
#include <Renderer/Shared/Utility/MultipleScatterLUTUtility.h>
#include <Renderer/Shared/CRenderer.h>
#include <thread>
#include <chrono>


const bool g_overrideLutCache = false;

namespace YAPT
{ 
	
	MultiScatteringLUTs::MultiScatteringLUTs()
		:m_renderer(nullptr)
	{

	}
	MultiScatteringLUTs::~MultiScatteringLUTs()
	{
		          
	}

	void MultiScatteringLUTs::initializeLUTStorage(CRenderer* renderer, RenderResourcesPool& pool)
	{
		m_renderer = renderer;
		{
			TextureDesc texDesc(ResourceDimension::TEXTURE_2D, MS_LUT_FORMAT, RESOURCE_USAGE_COPY_DESTINATION | RESOURCE_USAGE_SAMPLED_TEXTURE, SS_ALBEDO_LUT_DIM, SS_ALBEDO_LUT_DIM, 1, 1);
			ResourceStateDescription resState{ RESOURCE_USAGE_UNKNOWN, ACCESS_FLAGS_READ, SHADERSTAGE_NONE };
			m_ssDirAlbedoNoFresnelLUT.texture = pool.requestTexture(texDesc, resState, "SingleScatterDirectionalAlbedo");
		}
		{
			TextureDesc texDesc(ResourceDimension::TEXTURE_1D, MS_LUT_FORMAT, RESOURCE_USAGE_COPY_DESTINATION | RESOURCE_USAGE_SAMPLED_TEXTURE, SS_ALBEDO_LUT_DIM, 1, 1, 1);
			ResourceStateDescription resState{ RESOURCE_USAGE_UNKNOWN, ACCESS_FLAGS_READ, SHADERSTAGE_NONE };
			m_ssAvgDirAlbedoNoFresnelLUT.texture = pool.requestTexture(texDesc, resState, "SingleScatterAverageDirectionalAlbedo");
		}
		{
			TextureDesc texDesc(ResourceDimension::TEXTURE_3D, MS_LUT_FORMAT, RESOURCE_USAGE_COPY_DESTINATION | RESOURCE_USAGE_SAMPLED_TEXTURE, SSMS_ALBEDO_LUT_DIM, SSMS_ALBEDO_LUT_DIM, 1, SSMS_ALBEDO_LUT_DIM);
			ResourceStateDescription resState{ RESOURCE_USAGE_UNKNOWN, ACCESS_FLAGS_READ, SHADERSTAGE_NONE };
			m_ssmsDirAlbedoLUT.texture = pool.requestTexture(texDesc, resState, "SingleAndMultiScatterDirectionalAlbedo");
		}
		{
			TextureDesc texDesc(ResourceDimension::TEXTURE_2D, MS_LUT_FORMAT, RESOURCE_USAGE_COPY_DESTINATION | RESOURCE_USAGE_SAMPLED_TEXTURE, SSMS_ALBEDO_LUT_DIM, SSMS_ALBEDO_LUT_DIM, 1, 1);
			ResourceStateDescription resState{ RESOURCE_USAGE_UNKNOWN, ACCESS_FLAGS_READ, SHADERSTAGE_NONE };
			m_ssmsAvgDirAlbedoLUT.texture = pool.requestTexture(texDesc, resState, "SingleAndMultiScatterAverageDirectionalAlbedo");
		}
		{
			TextureDesc texDesc(ResourceDimension::TEXTURE_3D, MS_LUT_FORMAT, RESOURCE_USAGE_COPY_DESTINATION | RESOURCE_USAGE_SAMPLED_TEXTURE, SS_ALBEDO_TRANSLUCENT_LUT_DIM, SS_ALBEDO_TRANSLUCENT_LUT_DIM, 1, SS_ALBEDO_TRANSLUCENT_LUT_DIM);
			ResourceStateDescription resState{ RESOURCE_USAGE_UNKNOWN, ACCESS_FLAGS_READ, SHADERSTAGE_NONE };
			m_ssDirAlbedoTranslucentToDenserLUT.texture = pool.requestTexture(texDesc, resState, "SingleScatterDirectionalAlbedoTranslucentDenser");
		}
		{
			TextureDesc texDesc(ResourceDimension::TEXTURE_3D, MS_LUT_FORMAT, RESOURCE_USAGE_COPY_DESTINATION | RESOURCE_USAGE_SAMPLED_TEXTURE, SS_ALBEDO_TRANSLUCENT_LUT_DIM, SS_ALBEDO_TRANSLUCENT_LUT_DIM, 1, SS_ALBEDO_TRANSLUCENT_LUT_DIM);
			ResourceStateDescription resState{ RESOURCE_USAGE_UNKNOWN, ACCESS_FLAGS_READ, SHADERSTAGE_NONE };
			m_ssDirAlbedoTranslucentToLighterLUT.texture = pool.requestTexture(texDesc, resState, "SingleScatterDirectionalAlbedoTranslucentLighter");
		}
		{
			TextureDesc texDesc(ResourceDimension::TEXTURE_2D, MS_LUT_FORMAT, RESOURCE_USAGE_COPY_DESTINATION | RESOURCE_USAGE_SAMPLED_TEXTURE, SS_ALBEDO_TRANSLUCENT_LUT_DIM, SS_ALBEDO_TRANSLUCENT_LUT_DIM, 1, 1);
			ResourceStateDescription resState{ RESOURCE_USAGE_UNKNOWN, ACCESS_FLAGS_READ, SHADERSTAGE_NONE };
			m_ssAvgAlbedoTranslucentToDenserLUT.texture = pool.requestTexture(texDesc, resState, "SingleScatterAverageAlbedoTranslucentDenser");
		}
		{
			TextureDesc texDesc(ResourceDimension::TEXTURE_2D, MS_LUT_FORMAT, RESOURCE_USAGE_COPY_DESTINATION | RESOURCE_USAGE_SAMPLED_TEXTURE, SS_ALBEDO_TRANSLUCENT_LUT_DIM, SS_ALBEDO_TRANSLUCENT_LUT_DIM, 1, 1);
			ResourceStateDescription resState{ RESOURCE_USAGE_UNKNOWN, ACCESS_FLAGS_READ, SHADERSTAGE_NONE };
			m_ssAvgAlbedoTranslucentToLighterLUT.texture = pool.requestTexture(texDesc, resState, "SingleScatterAverageAlbedoTranslucentLighter");
		}
		{
			TextureDesc texDesc(ResourceDimension::TEXTURE_2D, MS_LUT_FORMAT, RESOURCE_USAGE_COPY_DESTINATION | RESOURCE_USAGE_SAMPLED_TEXTURE, SS_ALBEDO_SHEEN_LUT_DIM, SS_ALBEDO_SHEEN_LUT_DIM, 1, 1);
			ResourceStateDescription resState{ RESOURCE_USAGE_UNKNOWN, ACCESS_FLAGS_READ, SHADERSTAGE_NONE };
			m_dirAlbedoSheen.texture = pool.requestTexture(texDesc, resState, "DirectionalAlbedoSheen");
		}
		 
		
	}
	   

	void MultiScatteringLUTs::generateSingleScatterAlbedoLUTs(float* directionalAlbedoOUT, float* avgAlbedoOUT)
	{
		
			constexpr size_t WORKITEMSCOUNT = 8;
			struct WorkItems
			{
				float* singleScatterAlbedoOut;
				float* singleScatterAvgAlbedoOut;
				size_t from;
				size_t to;
			};

			std::array< WorkItems, WORKITEMSCOUNT> items;
			size_t steps = (SS_ALBEDO_LUT_DIM + WORKITEMSCOUNT - 1) / WORKITEMSCOUNT;
			std::array<JobHandle, WORKITEMSCOUNT> deps;

			for (size_t i = 0; i < WORKITEMSCOUNT; ++i)
			{
				items[i].singleScatterAlbedoOut = directionalAlbedoOUT;
				items[i].singleScatterAvgAlbedoOut = avgAlbedoOUT;
				items[i].from = i * steps;
				items[i].to = min((i + 1) * steps, size_t(SS_ALBEDO_LUT_DIM));
			}

			auto integrateSingleScatterLookups = [](void* usrData)
			{
				//directional albedo (no fresnel)
				WorkItems* items = static_cast<WorkItems*>(usrData);
				float* singleScatterAlbedo = items->singleScatterAlbedoOut;


				for (size_t y = items->from; y < items->to; ++y)
				{
					for (size_t x = 0; x < SS_ALBEDO_LUT_DIM; ++x)
					{
						float cosTheta = float(x) / (SS_ALBEDO_LUT_DIM - 1);
						float roughness = float(y) / (SS_ALBEDO_LUT_DIM - 1);

						float albedo = integrateDirectionalSingleScatterGGXAlbedoNoFresnel(roughness, max(cosTheta, MIN_COS_THETA));
						singleScatterAlbedo[y * SS_ALBEDO_LUT_DIM + x] = albedo;
					}
				}

				//average directional albedo
				float* singleScatterAvgAlbedo = items->singleScatterAvgAlbedoOut;

				for (size_t y = items->from; y < items->to; ++y)
				{
					float average = 0.f;
					for (size_t x = 0; x < SS_ALBEDO_LUT_DIM; ++x)
					{
						size_t srcIndex = y * SS_ALBEDO_LUT_DIM + x;
						float cosTheta = float(x) / (SS_ALBEDO_LUT_DIM - 1);
						cosTheta = max(cosTheta, MIN_COS_THETA);
						float dirAlbedo = singleScatterAlbedo[srcIndex];
						average += dirAlbedo * cosTheta;
					}
					singleScatterAvgAlbedo[y] = 2 * average / SS_ALBEDO_LUT_DIM; // multiply by PI intentionally omitted (done in the shader)
				}

			};

			for (size_t i = 0; i < WORKITEMSCOUNT; ++i)
			{
				deps[i] = m_renderer->getJobSystem().submit(integrateSingleScatterLookups, &items[i]);
			}


			m_renderer->getJobSystem().wait(m_renderer->getJobSystem().combineDependencies(deps.data(), WORKITEMSCOUNT));
		
	}

	void MultiScatteringLUTs::generateMultiScatterAlbedoLUTs(const float* directionalAlbedo, const float* avgAlbedo, float* directionalMultiScatterAlbedoOUT, float* avgMultiScatterAlbedoOUT)
	{
		struct WorkItems
		{
			float* fullScatteringAlbedoOut;
			float* fullScatteringAvgAlbedoOut;
			const float* singleScatteringAlbedo;
			const float* singleScatteringAvgAlbedo;

			size_t from;
			size_t to;
		};

		constexpr size_t WORKITEMSCOUNT = 8;
		std::array<WorkItems, WORKITEMSCOUNT> items;
		std::array<JobHandle, WORKITEMSCOUNT> deps;
		size_t steps = (SSMS_ALBEDO_LUT_DIM + WORKITEMSCOUNT - 1) / WORKITEMSCOUNT;

		for (size_t i = 0; i < WORKITEMSCOUNT; ++i)
		{
			items[i].fullScatteringAlbedoOut = directionalMultiScatterAlbedoOUT;
			items[i].fullScatteringAvgAlbedoOut = avgMultiScatterAlbedoOUT;
			items[i].singleScatteringAlbedo = directionalAlbedo;
			items[i].singleScatteringAvgAlbedo = avgAlbedo;

			items[i].from = i * steps;
			items[i].to = min((i + 1) * steps, size_t(SS_ALBEDO_LUT_DIM));
		}


		auto integrateFullScatteringLookups = [](void* usrData)
		{
			WorkItems* item = static_cast<WorkItems*>(usrData);

			//directional multiscatter albedo
			float* fullScatteringAlbedo = item->fullScatteringAlbedoOut;
			for (size_t z = item->from; z < item->to; ++z)
			{

				for (size_t y = 0; y < SSMS_ALBEDO_LUT_DIM; ++y)
				{

					float roughness = float(y) / (SSMS_ALBEDO_LUT_DIM - 1);
					for (size_t x = 0; x < SSMS_ALBEDO_LUT_DIM; ++x)
					{
						float cosTheta = float(x) / (SSMS_ALBEDO_LUT_DIM - 1);
						float etaR = float(z) / (SSMS_ALBEDO_LUT_DIM - 1);
						etaR = 1.f + etaR * 3.f;
						 
						etaR = max(etaR, 1.f + ETA_EPSILON_MIN_FROM_1);

						float albedo = integrateSingleAndMultiScatterGGXAlbedo(roughness, max(cosTheta, MIN_COS_THETA), etaR, item->singleScatteringAlbedo, item->singleScatteringAvgAlbedo);
						size_t index = z * SSMS_ALBEDO_LUT_DIM * SSMS_ALBEDO_LUT_DIM + (y * SSMS_ALBEDO_LUT_DIM) + x;
						fullScatteringAlbedo[index] = albedo;
					}
				}

			}

			//average multiscattering albedo
			float* fullScatteringAvgAlbedo = item->fullScatteringAvgAlbedoOut;
			for (size_t z = item->from; z < item->to; ++z)
			{
				for (size_t y = 0; y < SSMS_ALBEDO_LUT_DIM; ++y)
				{
					float average = 0.f;
					for (size_t x = 0; x < SSMS_ALBEDO_LUT_DIM; ++x)
					{
						float cosTheta = float(x) / (SSMS_ALBEDO_LUT_DIM - 1);
						cosTheta = max(cosTheta, MIN_COS_THETA);
						size_t srcIndex = z * SSMS_ALBEDO_LUT_DIM * SSMS_ALBEDO_LUT_DIM + (y * SSMS_ALBEDO_LUT_DIM) + x;
						float dirAlbedo = fullScatteringAlbedo[srcIndex];

						average += dirAlbedo * cosTheta;
					}

					fullScatteringAvgAlbedo[z * SSMS_ALBEDO_LUT_DIM + y] = 2 * average / SSMS_ALBEDO_LUT_DIM; // multiply by PI intentionally omitted (done in the shader)
				}

			}

		};
		              
		for (size_t i = 0; i < WORKITEMSCOUNT; ++i)
		{
			deps[i] = m_renderer->getJobSystem().submit(integrateFullScatteringLookups, &items[i]);
		}


		m_renderer->getJobSystem().wait(m_renderer->getJobSystem().combineDependencies(deps.data(), WORKITEMSCOUNT));
	}

	void MultiScatteringLUTs::generateDirectionalAlbedoSheenLUT(float* directionalAlbedoOUT)
	{
		constexpr size_t WORKITEMSCOUNT = 8;
		struct WorkItems
		{
			float* singleScatterAlbedoOut;

			size_t from;
			size_t to;
		};

		std::array< WorkItems, WORKITEMSCOUNT> items;
		std::array<JobHandle, WORKITEMSCOUNT> deps;
		size_t steps = (SS_ALBEDO_SHEEN_LUT_DIM + WORKITEMSCOUNT - 1) / WORKITEMSCOUNT;

		for (size_t i = 0; i < WORKITEMSCOUNT; ++i)
		{
			items[i].singleScatterAlbedoOut = directionalAlbedoOUT;

			items[i].from = i * steps;
			items[i].to = min((i + 1) * steps, size_t(SS_ALBEDO_SHEEN_LUT_DIM));
		}

		auto integrateSingleScatterLookups = [](void* usrData)
		{
			//directional albedo (no fresnel)
			WorkItems* items = static_cast<WorkItems*>(usrData);
			float* singleScatterAlbedo = items->singleScatterAlbedoOut;


			for (size_t y = items->from; y < items->to; ++y)
			{
				for (size_t x = 0; x < SS_ALBEDO_SHEEN_LUT_DIM; ++x)
				{
					float cosTheta = float(x) / (SS_ALBEDO_SHEEN_LUT_DIM - 1);
					float roughness = float(y) / (SS_ALBEDO_SHEEN_LUT_DIM - 1);

					float albedo = integrateDirectionalSingleScatterAlbedoSheenNoFresnel(roughness, max(cosTheta, MIN_COS_THETA));
					singleScatterAlbedo[y * SS_ALBEDO_SHEEN_LUT_DIM + x] = albedo;
				}
			}
		};

		for (size_t i = 0; i < WORKITEMSCOUNT; ++i)
		{
			deps[i] = m_renderer->getJobSystem().submit(integrateSingleScatterLookups, &items[i]);
		}

		m_renderer->getJobSystem().wait(m_renderer->getJobSystem().combineDependencies(deps.data(), deps.size()));

	}

	void MultiScatteringLUTs::generateTranslucentScatterAlbedoLUTs(float* directionalTranslucentScatterAlbedoDenserOUT, float* directionalTranslucentScatterAlbedoLighterOUT,
		float* avgTranslucentScatterAlbedoDenserOUT, float* avgTranslucentScatterAlbedoLighterOUT)
	{
		struct Batch
		{
			float* translucentScatterAlbedoOut;
			float* translucentAvgScatterAlbedoOut;
			size_t from;
			size_t to;
			float etaScale;
			float etaBias;
			float etaMin;
			float etaMax;
		};

		constexpr size_t BATCHES_COUNT = 8;
		std::array<JobHandle, BATCHES_COUNT * 2> deps;
		size_t steps = (SS_ALBEDO_TRANSLUCENT_LUT_DIM + BATCHES_COUNT - 1) / BATCHES_COUNT;

		std::array< Batch, BATCHES_COUNT> batchesDense;
		for (size_t i = 0; i < BATCHES_COUNT; ++i)
		{
			batchesDense[i].translucentScatterAlbedoOut = directionalTranslucentScatterAlbedoDenserOUT;
			batchesDense[i].translucentAvgScatterAlbedoOut = avgTranslucentScatterAlbedoDenserOUT;

			batchesDense[i].from = i * steps;
			batchesDense[i].to = min((i + 1) * steps, size_t(SS_ALBEDO_TRANSLUCENT_LUT_DIM));

			batchesDense[i].etaScale = 3.f;
			batchesDense[i].etaBias = 1.f;
			batchesDense[i].etaMin = 1.0001f;
			batchesDense[i].etaMax = 4.0f;
		}

		 
		auto integrateTranslucentScattering = [](void* usrData)
		{
			Batch* item = static_cast<Batch*>(usrData);

			//directional multiscatter albedo
			float* translucentScatterAlbedo = item->translucentScatterAlbedoOut;
			for (size_t z = item->from; z < item->to; ++z)
			{
				for (size_t y = 0; y < SS_ALBEDO_TRANSLUCENT_LUT_DIM; ++y)
				{
					float roughness = float(y) / (SS_ALBEDO_TRANSLUCENT_LUT_DIM - 1);
					for (size_t x = 0; x < SS_ALBEDO_TRANSLUCENT_LUT_DIM; ++x)
					{
						float cosTheta = float(x) / (SS_ALBEDO_TRANSLUCENT_LUT_DIM - 1);
						float etaR = float(z) / (SS_ALBEDO_TRANSLUCENT_LUT_DIM - 1);

						etaR = item->etaScale * etaR + item->etaBias;
						etaR = glm::clamp(etaR, item->etaMin, item->etaMax);

						float albedo = integrateDirectionalSingleScatterGGXAlbedoTranslucent(etaR, roughness, max(cosTheta, MIN_COS_THETA));
						size_t index = z * SS_ALBEDO_TRANSLUCENT_LUT_DIM * SS_ALBEDO_TRANSLUCENT_LUT_DIM + (y * SS_ALBEDO_TRANSLUCENT_LUT_DIM) + x;
						translucentScatterAlbedo[index] = albedo;
					}
				}
				     
			}                           
			//average  albedo
			float* AvgAlbedo = item->translucentAvgScatterAlbedoOut;
			for (size_t z = item->from; z < item->to; ++z)
			{
				for (size_t y = 0; y < SS_ALBEDO_TRANSLUCENT_LUT_DIM; ++y)
				{
					float average = 0.f;
					for (size_t x = 0; x < SS_ALBEDO_TRANSLUCENT_LUT_DIM; ++x)
					{
						float cosTheta = float(x) / (SS_ALBEDO_TRANSLUCENT_LUT_DIM - 1);
						size_t srcIndex = z * SS_ALBEDO_TRANSLUCENT_LUT_DIM * SS_ALBEDO_TRANSLUCENT_LUT_DIM + (y * SS_ALBEDO_TRANSLUCENT_LUT_DIM) + x;
						float dirAlbedo = translucentScatterAlbedo[srcIndex];
						cosTheta = max(cosTheta, MIN_COS_THETA);

						average += dirAlbedo * cosTheta;
					}

					AvgAlbedo[z * SS_ALBEDO_TRANSLUCENT_LUT_DIM + y] = 2 * average / SS_ALBEDO_TRANSLUCENT_LUT_DIM; // multiply by PI intentionally omitted (done in the shader)
				}

			}

		};

		for (size_t i = 0; i < BATCHES_COUNT; ++i)
		{
			deps[i] = m_renderer->getJobSystem().submit(integrateTranslucentScattering, &batchesDense[i]);
		}


		std::array< Batch, BATCHES_COUNT> batchesLight;
		for (size_t i = 0; i < BATCHES_COUNT; ++i)
		{
			batchesLight[i].translucentScatterAlbedoOut = directionalTranslucentScatterAlbedoLighterOUT;
			batchesLight[i].translucentAvgScatterAlbedoOut = avgTranslucentScatterAlbedoLighterOUT;

			batchesLight[i].from = i * steps;
			batchesLight[i].to = min((i + 1) * steps, size_t(SS_ALBEDO_TRANSLUCENT_LUT_DIM));

			batchesLight[i].etaScale = 0.69f;
			batchesLight[i].etaBias = 0.3f;
			batchesLight[i].etaMin = 0.3f;
			batchesLight[i].etaMax = 0.99f;
		}


		    
		for (size_t i = 0; i < BATCHES_COUNT; ++i)
		{
			deps[BATCHES_COUNT + i] = m_renderer->getJobSystem().submit(integrateTranslucentScattering, &batchesLight[i]);
		}

		m_renderer->getJobSystem().wait(m_renderer->getJobSystem().combineDependencies(deps.data(), deps.size()));
	}
        
	void MultiScatteringLUTs::initializeLUTContents()
	{
		std::vector<float> singleScatterDirAlbedo;
		singleScatterDirAlbedo.resize(SS_ALBEDO_LUT_DIM * SS_ALBEDO_LUT_DIM);
		     

		std::vector<float> singleScatterAvgDirAlbedo;
		singleScatterAvgDirAlbedo.resize(SS_ALBEDO_LUT_DIM);

		std::vector<float> singleAndMultiScatterDirAlbedo;
		singleAndMultiScatterDirAlbedo.resize(SSMS_ALBEDO_LUT_DIM * SSMS_ALBEDO_LUT_DIM * SSMS_ALBEDO_LUT_DIM);

		std::vector<float> multiScatterAvgDirAlbedo;
		multiScatterAvgDirAlbedo.resize(SSMS_ALBEDO_LUT_DIM * SSMS_ALBEDO_LUT_DIM);

		std::vector<float> singleScatterTranslucentDirAlbedoDenser;
		singleScatterTranslucentDirAlbedoDenser.resize(SS_ALBEDO_TRANSLUCENT_LUT_DIM * SS_ALBEDO_TRANSLUCENT_LUT_DIM * SS_ALBEDO_TRANSLUCENT_LUT_DIM);

		std::vector<float> singleScatterTranslucentDirAlbedoLighter;
		singleScatterTranslucentDirAlbedoLighter.resize(SS_ALBEDO_TRANSLUCENT_LUT_DIM * SS_ALBEDO_TRANSLUCENT_LUT_DIM * SS_ALBEDO_TRANSLUCENT_LUT_DIM);

		std::vector<float> singleScatterTranslucentAvgAlbedoDenser;
		singleScatterTranslucentAvgAlbedoDenser.resize(SS_ALBEDO_TRANSLUCENT_LUT_DIM * SS_ALBEDO_TRANSLUCENT_LUT_DIM);
		  
		std::vector<float> singleScatterTranslucentAvgAlbedoLighter;
		singleScatterTranslucentAvgAlbedoLighter.resize(SS_ALBEDO_TRANSLUCENT_LUT_DIM * SS_ALBEDO_TRANSLUCENT_LUT_DIM);

		std::vector<float> dirAlbedoSheen;
		dirAlbedoSheen.resize(SS_ALBEDO_SHEEN_LUT_DIM * SS_ALBEDO_SHEEN_LUT_DIM);

		if (!loadSingleScatterAlbedoLUTs(singleScatterDirAlbedo.data(), singleScatterAvgDirAlbedo.data()))
		{
			generateSingleScatterAlbedoLUTs(singleScatterDirAlbedo.data(), singleScatterAvgDirAlbedo.data());
			storeSingleScatterAlbedoLUTs(singleScatterDirAlbedo.data(), singleScatterAvgDirAlbedo.data());
		}

		if (!loadMultiScatterAlbedoLUTs(singleAndMultiScatterDirAlbedo.data(), multiScatterAvgDirAlbedo.data()))
		{
			generateMultiScatterAlbedoLUTs(singleScatterDirAlbedo.data(), singleScatterAvgDirAlbedo.data(), singleAndMultiScatterDirAlbedo.data(), multiScatterAvgDirAlbedo.data());
			storeMultiScatterAlbedoLUTs(singleAndMultiScatterDirAlbedo.data(), multiScatterAvgDirAlbedo.data());
		}
		
		if (!loadTranslucentScatterAlbedoLUTs(singleScatterTranslucentDirAlbedoDenser.data(), singleScatterTranslucentDirAlbedoLighter.data(),
			singleScatterTranslucentAvgAlbedoDenser.data(), singleScatterTranslucentAvgAlbedoLighter.data()))
		{
			generateTranslucentScatterAlbedoLUTs(singleScatterTranslucentDirAlbedoDenser.data(), singleScatterTranslucentDirAlbedoLighter.data(),
				singleScatterTranslucentAvgAlbedoDenser.data(), singleScatterTranslucentAvgAlbedoLighter.data());
			storeTranslucentScatterAlbedoLUTs(singleScatterTranslucentDirAlbedoDenser.data(), singleScatterTranslucentDirAlbedoLighter.data(),
				singleScatterTranslucentAvgAlbedoDenser.data(), singleScatterTranslucentAvgAlbedoLighter.data());
		}
		
		if (!loadDirectionalAlbedoSheenLUT(dirAlbedoSheen.data()))
		{
			generateDirectionalAlbedoSheenLUT(dirAlbedoSheen.data());
			storeDirectionalAlbedoSheenLUT(dirAlbedoSheen.data());
		}
		 
                                                                                                                             
		TextureDataDefinition texData;         
		texData.rowPitchInBytes = size_t(getFormatSizeInBytes(MS_LUT_FORMAT)) * SS_ALBEDO_LUT_DIM;
		texData.data = singleScatterDirAlbedo.data();
		Gfx::uploadTexture(m_renderer->getGfxHandle(), m_ssDirAlbedoNoFresnelLUT.texture, 0, 1, 0, 1, &texData, GpuUploadStage::BEFORE_RENDER);
		   
		texData.rowPitchInBytes = size_t(getFormatSizeInBytes(MS_LUT_FORMAT)) * SS_ALBEDO_LUT_DIM;
		texData.data = singleScatterAvgDirAlbedo.data();
		Gfx::uploadTexture(m_renderer->getGfxHandle(), m_ssAvgDirAlbedoNoFresnelLUT.texture, 0, 1, 0, 1, &texData, GpuUploadStage::BEFORE_RENDER);
		
		texData.rowPitchInBytes = size_t(getFormatSizeInBytes(MS_LUT_FORMAT)) * SSMS_ALBEDO_LUT_DIM;
		texData.data = singleAndMultiScatterDirAlbedo.data();
		Gfx::uploadTexture(m_renderer->getGfxHandle(), m_ssmsDirAlbedoLUT.texture, 0, 1, 0, 1, &texData, GpuUploadStage::BEFORE_RENDER);
		
		texData.rowPitchInBytes = size_t(getFormatSizeInBytes(MS_LUT_FORMAT)) * SSMS_ALBEDO_LUT_DIM;
		texData.data = multiScatterAvgDirAlbedo.data();
		Gfx::uploadTexture(m_renderer->getGfxHandle(), m_ssmsAvgDirAlbedoLUT.texture, 0, 1, 0, 1, &texData, GpuUploadStage::BEFORE_RENDER);

		texData.rowPitchInBytes = size_t(getFormatSizeInBytes(MS_LUT_FORMAT)) * SS_ALBEDO_TRANSLUCENT_LUT_DIM;
		texData.data = singleScatterTranslucentDirAlbedoDenser.data();
		Gfx::uploadTexture(m_renderer->getGfxHandle(), m_ssDirAlbedoTranslucentToDenserLUT.texture, 0, 1, 0, 1, &texData, GpuUploadStage::BEFORE_RENDER);

		texData.rowPitchInBytes = size_t(getFormatSizeInBytes(MS_LUT_FORMAT)) * SS_ALBEDO_TRANSLUCENT_LUT_DIM;
		texData.data = singleScatterTranslucentDirAlbedoLighter.data();
		Gfx::uploadTexture(m_renderer->getGfxHandle(), m_ssDirAlbedoTranslucentToLighterLUT.texture, 0, 1, 0, 1, &texData, GpuUploadStage::BEFORE_RENDER);
		
		texData.rowPitchInBytes = size_t(getFormatSizeInBytes(MS_LUT_FORMAT)) * SS_ALBEDO_TRANSLUCENT_LUT_DIM;
		texData.data = singleScatterTranslucentAvgAlbedoDenser.data();
		Gfx::uploadTexture(m_renderer->getGfxHandle(), m_ssAvgAlbedoTranslucentToDenserLUT.texture, 0, 1, 0, 1, &texData, GpuUploadStage::BEFORE_RENDER);

		texData.rowPitchInBytes = size_t(getFormatSizeInBytes(MS_LUT_FORMAT)) * SS_ALBEDO_TRANSLUCENT_LUT_DIM;
		texData.data = singleScatterTranslucentAvgAlbedoLighter.data();
		Gfx::uploadTexture(m_renderer->getGfxHandle(), m_ssAvgAlbedoTranslucentToLighterLUT.texture, 0, 1, 0, 1, &texData, GpuUploadStage::BEFORE_RENDER);

		texData.rowPitchInBytes = size_t(getFormatSizeInBytes(MS_LUT_FORMAT)) * SS_ALBEDO_SHEEN_LUT_DIM;
		texData.data = dirAlbedoSheen.data();
		Gfx::uploadTexture(m_renderer->getGfxHandle(), m_dirAlbedoSheen.texture, 0, 1, 0, 1, &texData, GpuUploadStage::BEFORE_RENDER);
		 
		       
		/*//check energy 
		{
			for (size_t y = 0; y < SS_ALBEDO_TRANSLUCENT_LUT_DIM; ++y)
			{
				float sumSS = 0.f;
				float sumMS = 0.f;

				float eta = 2.5f;
				float roughness = float(y) / (SS_ALBEDO_TRANSLUCENT_LUT_DIM - 1);
				for (size_t x = 0; x < SS_ALBEDO_TRANSLUCENT_LUT_DIM; ++x)
				{
					float cosTheta = float(x) / (SS_ALBEDO_TRANSLUCENT_LUT_DIM - 1);

					float ms = integrateDirectionalMultiScatterAlbedoTranslucent(eta, roughness, max(cosTheta, MIN_COS_THETA), singleScatterTranslucentDirAlbedoDenser.data(),
						singleScatterTranslucentAvgAlbedoDenser.data(), singleScatterTranslucentDirAlbedoLighter.data(), singleScatterTranslucentAvgAlbedoLighter.data());

					float ss = integrateDirectionalSingleScatterAlbedoTranslucent(eta, roughness, max(cosTheta, MIN_COS_THETA));

					sumSS += ss * cosTheta;
					sumMS += ms * cosTheta;
				}

				sumSS = 2 * PI * sumSS / SS_ALBEDO_TRANSLUCENT_LUT_DIM;
				sumMS = 2 * PI * sumMS / SS_ALBEDO_TRANSLUCENT_LUT_DIM;

				YAPT_LOG_DEBUG("Eta: %f, Roughness: %f, Sum SS: %f, Sum MS: %f Combined: %f", eta, roughness, sumSS, sumMS, sumSS + sumMS );

			}

		}*/
		                  
		m_ssDirAlbedoNoFresnelLUT.textureView = Gfx::getTextureView(m_renderer->getGfxHandle(), m_ssDirAlbedoNoFresnelLUT.texture, { MS_LUT_FORMAT });
		m_ssAvgDirAlbedoNoFresnelLUT.textureView = Gfx::getTextureView(m_renderer->getGfxHandle(), m_ssAvgDirAlbedoNoFresnelLUT.texture, { MS_LUT_FORMAT });
		m_ssmsDirAlbedoLUT.textureView = Gfx::getTextureView(m_renderer->getGfxHandle(), m_ssmsDirAlbedoLUT.texture, { MS_LUT_FORMAT });
		m_ssmsAvgDirAlbedoLUT.textureView = Gfx::getTextureView(m_renderer->getGfxHandle(), m_ssmsAvgDirAlbedoLUT.texture, { MS_LUT_FORMAT });
		m_ssDirAlbedoTranslucentToDenserLUT.textureView = Gfx::getTextureView(m_renderer->getGfxHandle(), m_ssDirAlbedoTranslucentToDenserLUT.texture, { MS_LUT_FORMAT });
		m_ssDirAlbedoTranslucentToLighterLUT.textureView = Gfx::getTextureView(m_renderer->getGfxHandle(), m_ssDirAlbedoTranslucentToLighterLUT.texture, { MS_LUT_FORMAT });
		m_ssAvgAlbedoTranslucentToDenserLUT.textureView = Gfx::getTextureView(m_renderer->getGfxHandle(), m_ssAvgAlbedoTranslucentToDenserLUT.texture, { MS_LUT_FORMAT });
		m_ssAvgAlbedoTranslucentToLighterLUT.textureView = Gfx::getTextureView(m_renderer->getGfxHandle(), m_ssAvgAlbedoTranslucentToLighterLUT.texture, { MS_LUT_FORMAT });
		
		m_dirAlbedoSheen.textureView = Gfx::getTextureView(m_renderer->getGfxHandle(), m_dirAlbedoSheen.texture, { MS_LUT_FORMAT });
	}

	bool MultiScatteringLUTs::loadSingleScatterAlbedoLUTs(float* directionalAlbedoOUT, float* avgAlbedoOUT)
	{
		if (g_overrideLutCache)
		{
			return false;
		}

		bool success = tryToLoadFromfile("DirectionalAlbedoNoFresnel", ResourceDimension::TEXTURE_2D, glm::uvec3(SS_ALBEDO_LUT_DIM, SS_ALBEDO_LUT_DIM, 1), directionalAlbedoOUT);
		bool success2 = tryToLoadFromfile("AverageAlbedoNoFresnel", ResourceDimension::TEXTURE_1D, glm::uvec3(SS_ALBEDO_LUT_DIM, 1, 1), avgAlbedoOUT);

		return success && success2;
	}
	bool MultiScatteringLUTs::loadMultiScatterAlbedoLUTs(float* directionalMultiScatterAlbedoOUT, float* avgMultiScatterAlbedoOUT)
	{
		if (g_overrideLutCache)
		{
			return false;
		}
		bool success = tryToLoadFromfile("DirectionalMultiscatteringAlbedo", ResourceDimension::TEXTURE_3D, glm::uvec3(SSMS_ALBEDO_LUT_DIM, SSMS_ALBEDO_LUT_DIM, SSMS_ALBEDO_LUT_DIM), directionalMultiScatterAlbedoOUT);
		bool success2 = tryToLoadFromfile("AverageMultiscatteringAlbedo", ResourceDimension::TEXTURE_2D, glm::uvec3(SSMS_ALBEDO_LUT_DIM, SSMS_ALBEDO_LUT_DIM, 1), avgMultiScatterAlbedoOUT);

		return success && success2;
	}
	bool MultiScatteringLUTs::loadTranslucentScatterAlbedoLUTs(float* directionalTranslucentScatterAlbedoDenserOUT, float* directionalTranslucentScatterAlbedoLighterOUT,
		float* avgTranslucentScatterAlbedoDenserOUT, float* avgTranslucentScatterAlbedoLighterOUT)
	{
		if (g_overrideLutCache)
		{
			return false;
		}

		bool success = tryToLoadFromfile("DirectionalAlbedoTranslucentDenser", ResourceDimension::TEXTURE_3D,
			glm::uvec3(SS_ALBEDO_TRANSLUCENT_LUT_DIM, SS_ALBEDO_TRANSLUCENT_LUT_DIM, SS_ALBEDO_TRANSLUCENT_LUT_DIM), directionalTranslucentScatterAlbedoDenserOUT);

		bool success2 = tryToLoadFromfile("AverageAlbedoTranslucentDenser", ResourceDimension::TEXTURE_2D, glm::uvec3(SS_ALBEDO_TRANSLUCENT_LUT_DIM, SS_ALBEDO_TRANSLUCENT_LUT_DIM, 1), avgTranslucentScatterAlbedoDenserOUT);

		bool success3 = tryToLoadFromfile("DirectionalAlbedoTranslucentLighter", ResourceDimension::TEXTURE_3D,
			glm::uvec3(SS_ALBEDO_TRANSLUCENT_LUT_DIM, SS_ALBEDO_TRANSLUCENT_LUT_DIM, SS_ALBEDO_TRANSLUCENT_LUT_DIM), directionalTranslucentScatterAlbedoLighterOUT);

		bool success4 = tryToLoadFromfile("AverageAlbedoTranslucentLighter", ResourceDimension::TEXTURE_2D, glm::uvec3(SS_ALBEDO_TRANSLUCENT_LUT_DIM, SS_ALBEDO_TRANSLUCENT_LUT_DIM, 1), avgTranslucentScatterAlbedoLighterOUT);

		return success && success2 && success3 && success4;
	}

	bool MultiScatteringLUTs::loadDirectionalAlbedoSheenLUT(float* directionalAlbedoOUT)
	{
		if (g_overrideLutCache)
		{
			return false;
		}
		  
		bool success = tryToLoadFromfile("DirectionalAlbedoSheen", ResourceDimension::TEXTURE_2D, glm::uvec3(SS_ALBEDO_SHEEN_LUT_DIM, SS_ALBEDO_SHEEN_LUT_DIM, 1), directionalAlbedoOUT);
		return success;
	}

	void MultiScatteringLUTs::storeSingleScatterAlbedoLUTs(const float* directionalAlbedoOUT, const float* avgAlbedoOUT)
	{
		RendererCacheProvider* cache = m_renderer->getCacheProvider();
		if (!cache)
		{
			return;
		}
		RendererCacheProvider::ImageDefinition def;
		def.width = SS_ALBEDO_LUT_DIM;
		def.height = SS_ALBEDO_LUT_DIM;
		def.depthOrSlices = 1;
		def.format = MS_LUT_FORMAT;
		def.dimension = ResourceDimension::TEXTURE_2D;
		def.mips = 1;

		cache->storeImage("DirectionalAlbedoNoFresnel", def, directionalAlbedoOUT);

		def.height = 1;
		def.dimension = ResourceDimension::TEXTURE_1D;

		cache->storeImage("AverageAlbedoNoFresnel", def, avgAlbedoOUT);

	}
	void MultiScatteringLUTs::storeMultiScatterAlbedoLUTs(const float* directionalMultiScatterAlbedoOUT, const float* avgMultiScatterAlbedoOUT)
	{
		RendererCacheProvider* cache = m_renderer->getCacheProvider();
		if (!cache)
		{
			return;
		}

		RendererCacheProvider::ImageDefinition def;
		def.width = SSMS_ALBEDO_LUT_DIM;
		def.height = SSMS_ALBEDO_LUT_DIM;
		def.depthOrSlices = SSMS_ALBEDO_LUT_DIM;
		def.format = MS_LUT_FORMAT;
		def.dimension = ResourceDimension::TEXTURE_3D;
		def.mips = 1;
		cache->storeImage("DirectionalMultiscatteringAlbedo", def, directionalMultiScatterAlbedoOUT);

		def.depthOrSlices = 1;
		def.dimension = ResourceDimension::TEXTURE_2D;
		cache->storeImage("AverageMultiscatteringAlbedo", def, avgMultiScatterAlbedoOUT);
	}
	void MultiScatteringLUTs::storeTranslucentScatterAlbedoLUTs(const float* directionalTranslucentScatterAlbedoDenserOUT, const float* directionalTranslucentScatterAlbedoLighterOUT,
		const float* avgTranslucentScatterAlbedoDenserOUT, const float* avgTranslucentScatterAlbedoLighterOUT)
	{
		RendererCacheProvider* cache = m_renderer->getCacheProvider();
		if (!cache)
		{
			return;
		}
		        
		RendererCacheProvider::ImageDefinition def;
		def.width = SS_ALBEDO_TRANSLUCENT_LUT_DIM;
		def.height = SS_ALBEDO_TRANSLUCENT_LUT_DIM;
		def.depthOrSlices = SS_ALBEDO_TRANSLUCENT_LUT_DIM;
		def.format = MS_LUT_FORMAT;
		def.dimension = ResourceDimension::TEXTURE_3D;
		def.mips = 1;
		cache->storeImage("DirectionalAlbedoTranslucentDenser", def, directionalTranslucentScatterAlbedoDenserOUT);
		cache->storeImage("DirectionalAlbedoTranslucentLighter", def, directionalTranslucentScatterAlbedoLighterOUT);

		def.depthOrSlices = 1;
		def.dimension = ResourceDimension::TEXTURE_2D;
		cache->storeImage("AverageAlbedoTranslucentDenser", def, avgTranslucentScatterAlbedoDenserOUT);
		cache->storeImage("AverageAlbedoTranslucentLighter", def, avgTranslucentScatterAlbedoLighterOUT);

	}
	void MultiScatteringLUTs::storeDirectionalAlbedoSheenLUT(float* directionalAlbedoOUT)
	{
		RendererCacheProvider* cache = m_renderer->getCacheProvider();
		if (!cache)
		{
			return;
		}
		RendererCacheProvider::ImageDefinition def;
		def.width = SS_ALBEDO_SHEEN_LUT_DIM;
		def.height = SS_ALBEDO_SHEEN_LUT_DIM;
		def.depthOrSlices = 1;
		def.format = MS_LUT_FORMAT;
		def.dimension = ResourceDimension::TEXTURE_2D;
		def.mips = 1;

		cache->storeImage("DirectionalAlbedoSheen", def, directionalAlbedoOUT);

	}

	bool MultiScatteringLUTs::tryToLoadFromfile(const char* filename, ResourceDimension dim, const glm::uvec3& expectedDimensions, float* dataOut)
	{
		RendererCacheProvider* cache = m_renderer->getCacheProvider();
		if (!cache)
		{
			return false;
		}

		RendererCacheProvider::ImageDefinition def;
		def.width = expectedDimensions.x;
		def.height = expectedDimensions.y;
		def.depthOrSlices = expectedDimensions.z;
		def.format = MS_LUT_FORMAT;
		def.dimension = dim;
		def.mips = 1;


		bool succesful = cache->loadImage(filename, def, dataOut);


		return succesful;
	}

}