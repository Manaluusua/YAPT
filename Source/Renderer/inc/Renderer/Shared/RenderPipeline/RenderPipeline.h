#ifndef YAPT_SHARED_RENDERPIPELINE_H
#define YAPT_SHARED_RENDERPIPELINE_H

#include <Gfx/GfxApi.h>

namespace YAPT
{
	class RenderStage;
	class CRenderer;
	class ThreadPool;
	class RenderPipeline
	{
	public:

		struct InitializeContext
		{
			CRenderer* renderer;
		};

		struct PrepareContext
		{
			ThreadPool* prepareTasksPool;
			SwapChainHandle swapChain;
			size_t renderWidth;
			size_t renderHeight;
		};

		struct UpdateContext
		{
			ThreadPool* updateTasksPool;
		};

		virtual ~RenderPipeline() {}

		virtual void initialize(const InitializeContext& cntx) = 0;
		virtual void shutdown() = 0;
		virtual void prepare(const PrepareContext& cntx) = 0;
		virtual void update(const UpdateContext& cntx) = 0;
		virtual void execute() = 0;
		
		
	};
}
#endif