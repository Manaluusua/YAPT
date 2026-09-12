#ifndef YAPT_SHARED_RENDERPIPELINE_H
#define YAPT_SHARED_RENDERPIPELINE_H

#include <Gfx/GfxApi.h>
#include <Common/JobSystem.h>

namespace YAPT
{
	class RenderStage;
	class CRenderer;
	class RenderPipeline
	{
	public:

		enum class NamedRenderPipelineTexture
		{
			FinalColor
		};

		enum class NamedRenderPipelineBuffer
		{
			
		};

		struct InitializeContext
		{
			CRenderer* renderer;
		};

		struct PrepareContext
		{
			JobSystem* prepareTasksPool;
			SwapChainHandle swapChain;
			size_t renderWidth;
			size_t renderHeight;
		};

		struct UpdateContext
		{
			JobSystem* updateTasksPool;
		};

		struct ExecuteContext
		{
			JobSystem* executeTasksPool;
		};

		virtual ~RenderPipeline() {}

		virtual void initialize(const InitializeContext& cntx) = 0;
		virtual void shutdown() = 0;
		virtual JobHandle prepare(const PrepareContext& cntx) = 0;
		virtual JobHandle update(const UpdateContext& cntx) = 0;
		virtual void execute(const ExecuteContext& cntx) = 0;
		
		virtual TextureHandle getNamedResource(NamedRenderPipelineTexture tex) { return YAPT_NULL_HANDLE; };
		virtual BufferHandle getNamedResource(NamedRenderPipelineBuffer buff) { return YAPT_NULL_HANDLE; };
	};
}
#endif