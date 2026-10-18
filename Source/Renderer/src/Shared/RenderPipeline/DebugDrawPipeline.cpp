#include <Renderer/Shared/RenderPipeline/DebugDrawPipeline.h>
#include <Renderer/Shared/RenderPipeline/MeshDebugStage.h>
#include <Renderer/Shared/RenderPipeline/SwapChainStage.h>

namespace YAPT
{

	DebugDrawPipeline::DebugDrawPipeline()
		:m_meshDebugStage(nullptr),
		m_swapChainStage(nullptr)
	{

	}
	DebugDrawPipeline::~DebugDrawPipeline()
	{

	}


	void DebugDrawPipeline::createStages()
	{
		m_meshDebugStage = new MeshDebugStage(true);
		m_swapChainStage = new SwapChainStage;

		addRenderStage(m_meshDebugStage);
		addRenderStage(m_swapChainStage);

		m_numberOfCommandBuffers = 1;
	}

	void DebugDrawPipeline::connectStages()
	{
		m_swapChainStage->setInputConnection(SwapChainStage::SWAPCHAIN_STAGE_CONNECTION_COLOR, m_meshDebugStage->getOutputConnection(MeshDebugStage::MESHDEBUG_STAGE_CONNECTION_COLOR));
	}
}