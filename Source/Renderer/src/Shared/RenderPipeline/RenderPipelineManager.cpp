#include <Renderer/Shared/RenderPipeline/RenderPipelineManager.h>
#include <Renderer/Shared/RenderPipeline/PathTracer/PathTracerPipeline.h>
#include <Renderer/Shared/RenderPipeline/DebugDrawPipeline.h>
#include <Renderer/Shared/CRenderer.h>
#include <algorithm>
namespace YAPT
{
	const size_t PIPELINE_INDEX_PT = 0;
	const size_t PIPELINE_INDEX_BDPT = 1;
	const size_t PIPELINE_INDEX_DEBUG = 2;

	const int32_t MAIN_PIPELINE_PRIORITY = 0;

	RenderPipelineManager::RenderPipelineManager()
		:m_mainPipeline(nullptr),
		m_renderer(nullptr),
		m_lastSelectedRenderPipelineIndex(-1)
	{

	}

	void RenderPipelineManager::initialize(const RenderPipeline::InitializeContext& cntx)
	{
		m_pipelines.push_back(new PathTracerPipeline(PathTraceStage::PathIntegratorType::BACKWARDS));
		m_pipelines.push_back(new PathTracerPipeline(PathTraceStage::PathIntegratorType::BACKWARDS_WITH_NEE));
		m_pipelines.push_back(new PathTracerPipeline(PathTraceStage::PathIntegratorType::BIDIRECTIONAL));
		m_pipelines.push_back(new DebugDrawPipeline());

		m_renderer = cntx.renderer;
		m_lastSelectedRenderPipelineIndex = -1;

		checkPipelineChange();

	}
	void RenderPipelineManager::shutdown()
	{
		while (!m_activePipelines.empty())
		{
			deactivatePipeline(m_activePipelines.back().pipeline);
		}
		m_mainPipeline = nullptr;

		for (auto pipeline : m_pipelines)
		{
			delete pipeline;
		}

		m_pipelines.clear();
	}

	void RenderPipelineManager::checkPipelineChange()
	{
		//TODO: do this properly
		CRendererConfiguration& config = m_renderer->getConcreteRendererConfiguration();
		int32_t currentPipeline = config.getRendererVarValueInternal<int32_t>(RVARNAME_ACTIVE_RENDERPIPELINE);

		if (currentPipeline == m_lastSelectedRenderPipelineIndex) return;

		auto options = config.getRendererVariable(RVARNAME_ACTIVE_RENDERPIPELINE)->as<RenderVariableOptions>()->getOptions();
		const char* pipelineToActivate = options[currentPipeline];

		size_t pipelineIndex = -1;

		if(std::strcmp(pipelineToActivate, "BPT") == 0)
		{
			pipelineIndex = 0;
		}
		else if (std::strcmp(pipelineToActivate, "BPT_WITH_NEE") == 0)
		{
			pipelineIndex = 1;
		}
		else if (std::strcmp(pipelineToActivate, "BDPT") == 0)
		{
			pipelineIndex = 2;
		}
		else if (std::strcmp(pipelineToActivate, "DBG_Normals") == 0)
		{
			pipelineIndex = 3;
		}
		else if (std::strcmp(pipelineToActivate, "DBG_Tangents") == 0)
		{
			pipelineIndex = 3;
		}
		else if (std::strcmp(pipelineToActivate, "DBG_UV") == 0)
		{
			pipelineIndex = 3;
		}

		if (pipelineIndex == size_t(-1)) return;

		if (m_pipelines[pipelineIndex] == m_mainPipeline)
		{

			if (pipelineIndex == PIPELINE_INDEX_DEBUG)
			{
				//TODO: setup debug pipeline
			}

		}
		else
		{
			if (m_mainPipeline != nullptr)
			{
				Gfx::waitForDeviceIdle(m_renderer->getGfxHandle());
				deactivatePipeline(m_mainPipeline);
			}
			m_mainPipeline = m_pipelines[pipelineIndex];
			activatePipeline(m_mainPipeline, MAIN_PIPELINE_PRIORITY);
		}

		

		m_lastSelectedRenderPipelineIndex = currentPipeline;
	}

	JobHandle RenderPipelineManager::prepare(const RenderPipeline::PrepareContext& cntx)
	{
		checkPipelineChange();
		m_jobHandles.clear();
		for (ActivePipeline& active : m_activePipelines)
		{
			JobHandle h = active.pipeline->prepare(cntx);
			if (h.isValid())
			{
				m_jobHandles.push_back(h);
			}
		}
		return combineCollectedJobHandles(cntx.prepareTasksPool);
	}

	JobHandle RenderPipelineManager::update(const RenderPipeline::UpdateContext& cntx)
	{
		m_jobHandles.clear();
		for (ActivePipeline& active : m_activePipelines)
		{
			JobHandle h = active.pipeline->update(cntx);
			if (h.isValid())
			{
				m_jobHandles.push_back(h);
			}
		}
		return combineCollectedJobHandles(cntx.updateTasksPool);
	}

	void RenderPipelineManager::execute(const RenderPipeline::ExecuteContext& cntx)
	{
		if (m_activePipelines.empty()) return;

		for (ActivePipeline& active : m_activePipelines)
		{
			active.output.commandBuffers.clear();
			active.pipeline->prepareExecute(cntx);
		}

		if (m_activePipelines.size() == 1)
		{
			m_activePipelines[0].pipeline->execute(cntx, m_activePipelines[0].output);
		}
		else
		{
			auto executeJob = [](void* usrData)
			{
				ExecuteJobData* data = static_cast<ExecuteJobData*>(usrData);
				data->pipeline->execute(*data->cntx, *data->output);
			};

			m_executeJobData.resize(m_activePipelines.size());
			m_jobHandles.clear();
			for (size_t i = 0; i < m_activePipelines.size(); ++i)
			{
				m_executeJobData[i] = { m_activePipelines[i].pipeline, &cntx, &m_activePipelines[i].output };
				m_jobHandles.push_back(cntx.executeTasksPool->submit(executeJob, &m_executeJobData[i]));
			}
			cntx.executeTasksPool->wait(combineCollectedJobHandles(cntx.executeTasksPool));
		}

		GfxApiHandle gfxHandle = m_renderer->getGfxHandle();
		for (ActivePipeline& active : m_activePipelines)
		{
			if (!active.output.commandBuffers.empty())
			{
				Gfx::submitCommandBuffers(gfxHandle, active.output.commandBuffers.data(), active.output.commandBuffers.size());
			}
		}

		for (ActivePipeline& active : m_activePipelines)
		{
			active.pipeline->afterSubmit();
		}
	}

	void RenderPipelineManager::readbackRequested(ReadbackTarget target)
	{
		//TODO
	}
	TextureHandle RenderPipelineManager::getReadbackTextureResource(ReadbackTarget target)
	{
		if (m_mainPipeline)
		{
			return m_mainPipeline->getNamedResource(RenderPipeline::NamedRenderPipelineTexture::FinalColor);
		}
		return YAPT_NULL_HANDLE;
	}
	BufferHandle RenderPipelineManager::getReadbackBufferResource(ReadbackTarget target)
	{
		//TODO
		return YAPT_NULL_HANDLE;
	}

	void RenderPipelineManager::activatePipeline(RenderPipeline* p, int32_t priority)
	{

		RenderPipeline::InitializeContext cntx;
		cntx.renderer = m_renderer;
		p->initialize(cntx);

		ActivePipeline active;
		active.pipeline = p;
		active.priority = priority;

		auto insertPos = std::upper_bound(m_activePipelines.begin(), m_activePipelines.end(), priority,
			[](int32_t prio, const ActivePipeline& a) { return prio < a.priority; });
		m_activePipelines.insert(insertPos, std::move(active));
	}
	void RenderPipelineManager::deactivatePipeline(RenderPipeline* p)
	{
		if (!p) return;

		auto iter = std::find_if(m_activePipelines.begin(), m_activePipelines.end(),
			[p](const ActivePipeline& a) { return a.pipeline == p; });
		if (iter == m_activePipelines.end()) return;

		p->shutdown();
		m_activePipelines.erase(iter);
	}

	JobHandle RenderPipelineManager::combineCollectedJobHandles(JobSystem* jobSystem)
	{
		if (m_jobHandles.empty()) return {};
		if (m_jobHandles.size() == 1) return m_jobHandles[0];
		return jobSystem->combineDependencies(m_jobHandles.data(), m_jobHandles.size());
	}
}
