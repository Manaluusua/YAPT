#include <Renderer/Shared/RenderPipeline/RenderPipelineManager.h>
#include <Renderer/Shared/RenderPipeline/PathTracer/PathTracerPipeline.h>
#include <Renderer/Shared/RenderPipeline/DebugDrawPipeline.h>
#include <Renderer/Shared/CRenderer.h>
namespace YAPT
{
	const size_t PIPELINE_INDEX_PT = 0;
	const size_t PIPELINE_INDEX_BDPT = 1;
	const size_t PIPELINE_INDEX_DEBUG = 2;

	void RenderPipelineManager::initialize(const RenderPipeline::InitializeContext& cntx)
	{
		m_pipelines.push_back(new PathTracerPipeline(PathTraceStage::PathIntegratorType::BACKWARDS));
		m_pipelines.push_back(new PathTracerPipeline(PathTraceStage::PathIntegratorType::BIDIRECTIONAL));
		m_pipelines.push_back(new DebugDrawPipeline());

		m_renderer = cntx.renderer;
		m_lastSelectedRenderPipelineIndex = -1;

		checkPipelineChange();

	}
	void RenderPipelineManager::shutdown()
	{
		deactivatePipeline(m_activePipeline);

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

		if(std::strcmp(pipelineToActivate, "SimplePT") == 0)
		{
			pipelineIndex = 0;
		}
		else if (std::strcmp(pipelineToActivate, "BDPT") == 0)
		{
			pipelineIndex = 1;
		}
		else if (std::strcmp(pipelineToActivate, "DBG_Normals") == 0)
		{
			pipelineIndex = 2;
		}
		else if (std::strcmp(pipelineToActivate, "DBG_Tangents") == 0)
		{
			pipelineIndex = 2;
		}
		else if (std::strcmp(pipelineToActivate, "DBG_UV") == 0)
		{
			pipelineIndex = 2;
		}

		if (pipelineIndex == size_t(-1)) return;

		if (m_pipelines[pipelineIndex] == m_activePipeline)
		{
			
			if (pipelineIndex == PIPELINE_INDEX_DEBUG)
			{
				//TODO: setup debug pipeline
			}
			
		}
		else
		{
			if (m_activePipeline != nullptr)
			{
				Gfx::waitForDeviceIdle(m_renderer->getGfxHandle());
				deactivatePipeline(m_activePipeline);
			}
			m_activePipeline = m_pipelines[pipelineIndex];
			activatePipeline(m_activePipeline);
		}

		

		m_lastSelectedRenderPipelineIndex = currentPipeline;
	}

	void RenderPipelineManager::prepare(const RenderPipeline::PrepareContext& cntx)
	{
		checkPipelineChange();
		if (m_activePipeline)
		{
			m_activePipeline->prepare(cntx);
		}
	}

	void RenderPipelineManager::update(const RenderPipeline::UpdateContext& cntx)
	{
		if (m_activePipeline)
		{
			m_activePipeline->update(cntx);
		}
	}

	void RenderPipelineManager::execute()
	{
		if (m_activePipeline)
		{
			m_activePipeline->execute();
		}
	}


	void RenderPipelineManager::activatePipeline(RenderPipeline* p)
	{

		RenderPipeline::InitializeContext cntx;
		cntx.renderer = m_renderer;
		p->initialize(cntx);

	}
	void RenderPipelineManager::deactivatePipeline(RenderPipeline* p)
	{
		if (!p) return;
		p->shutdown();
	}
}