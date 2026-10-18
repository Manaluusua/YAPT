#ifndef YAPT_SHARED_RENDERSTAGE_H
#define YAPT_SHARED_RENDERSTAGE_H

#include <Gfx/GfxApi.h>
#include <Gfx/RenderGraph/RenderGraphCommon.h>
#include <Common/JobSystem.h>
#include <vector>

namespace YAPT
{
	class CRenderer;
	class RenderGraph;
	class RenderGraphNode;
	class RenderResourcesPool;

	struct RenderStageConnection
	{
		RenderGraphNode* node;
		size_t slot;
	};

	class RenderStage
	{
	public:

		struct InitializeData
		{
			RenderResourcesPool* pipelineLifetimeResources;
		};

		struct RenderGraphLifetimeData
		{
			RenderResourcesPool* renderGraphLifetimeResources;
		};

		struct RenderResolutionDependantResourcesData
		{
			uint32_t newRenderResolutionWidth;
			uint32_t newRenderResolutionHeight;
			RenderResourcesPool* resolutionDependantResourcesPool;
			bool resolutionChanged;
		};

		struct PrepareData
		{
			JobSystem* prepareTasksPool;
			SwapChainHandle swapChain;
		};

		struct UpdateData
		{
			JobSystem* updateTasksPool;
		};

		struct ExecuteData
		{
			std::vector<CommandBufferHandle>* commandBuffersOut;
		};

		void setup(CRenderer* renderer, RenderGraph* renderGraph)
		{
			m_renderer = renderer;
			m_renderGraph = renderGraph;
		}

		virtual void initialize(const InitializeData& data) = 0;
		virtual void createNodes() = 0;
		virtual bool needsRecompile() { return false; }

		virtual RenderStageConnection getOutputConnection(size_t id) = 0;
		virtual void setInputConnection(size_t id, const RenderStageConnection& connection) = 0;
		virtual void onRenderGraphCompiled(const RenderGraphLifetimeData& data) {};
		//also called after every recompile (resolutionChanged == false) since resource ids change
		virtual void onRenderResolutionChanged(const RenderResolutionDependantResourcesData& data) {};
		virtual void shutdown() = 0;

		virtual JobHandle prepare(const PrepareData& data) = 0;
		virtual JobHandle update(const UpdateData& data) = 0;
		virtual void beforeExecute(const ExecuteData& data) = 0;

		CRenderer* getRenderer() { return m_renderer; }
		RenderGraph* getGraph() { return m_renderGraph; }
		virtual ~RenderStage() {}
	private:
		CRenderer* m_renderer;
		RenderGraph* m_renderGraph;
		
	};
}
#endif