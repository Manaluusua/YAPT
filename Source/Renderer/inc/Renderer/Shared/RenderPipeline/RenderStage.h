#ifndef YAPT_SHARED_RENDERSTAGE_H
#define YAPT_SHARED_RENDERSTAGE_H

#include <Gfx/GfxApi.h>
#include <Gfx/RenderGraph/RenderGraphCommon.h>

namespace YAPT
{
	class CRenderer;
	class RenderGraph;
	class RenderGraphNode;
	class RenderResourcesPool;
	class ThreadPool;

	struct RenderStageConnection
	{
		RenderGraphNode* node;
		size_t slot;
	};

	class RenderStage
	{
	public:

		struct RenderGraphLifetimeData
		{
			RenderResourcesPool* renderGraphLifetimeResources;
		};

		struct RenderResolutionDependantResourcesData
		{
			uint32_t newRenderResolutionWidth;
			uint32_t newRenderResolutionHeight;
			RenderResourcesPool* resolutionDependantResourcesPool;
		};

		struct PrepareData
		{
			ThreadPool* prepareTasksPool;
			SwapChainHandle swapChain;
		};

		struct UpdateData
		{
			ThreadPool* updateTasksPool;
		};

		void setup(CRenderer* renderer, RenderGraph* renderGraph)
		{
			m_renderer = renderer;
			m_renderGraph = renderGraph;
		}

		virtual void initialize() = 0;

		virtual RenderStageConnection getOutputConnection(size_t id) = 0;
		virtual void setInputConnection(size_t id, const RenderStageConnection& connection) = 0;
		virtual void onRenderGraphCompiled(const RenderGraphLifetimeData& data) {};
		virtual void onRenderResolutionChanged(const RenderResolutionDependantResourcesData& data) {};
		virtual void shutdown() = 0;

		virtual void prepare(const PrepareData& data) = 0;
		virtual void update(const UpdateData& data) = 0;
		virtual void beforeExecute() = 0;

		CRenderer* getRenderer() { return m_renderer; }
		RenderGraph* getGraph() { return m_renderGraph; }
		virtual ~RenderStage() {}
	private:
		CRenderer* m_renderer;
		RenderGraph* m_renderGraph;
		
	};
}
#endif