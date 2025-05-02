#include <Renderer/Shared/RenderPipeline/RenderPipelineManager.h>
#include <Renderer/Shared/RenderPipeline/PathTracer/PathTracerPipeline.h>
#include <Renderer/Shared/RenderPipeline/DebugDrawPipeline.h>
namespace YAPT
{
	void RenderPipelineManager::initialize(const RenderPipeline::InitializeContext& cntx)
	{
		m_pipelines.push_back(new PathTracerPipeline());
		m_pipelines.push_back(new DebugDrawPipeline());

		m_renderer = cntx.renderer;

		m_activePipeline = m_pipelines[0];
		activatePipeline(m_activePipeline);


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

	void RenderPipelineManager::prepare(const RenderPipeline::PrepareContext& cntx)
	{
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