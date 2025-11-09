#include <Renderer/Shared/RenderPipeline/DenoiseStage.h>
#include <Gfx/RenderGraph/RenderGraph.h>
#include <Renderer/Shared/Utility/RenderResourcesPool.h>
#include <Gfx/GfxApi.h>
#include <Renderer/Shared/CRenderer.h>
#include <Renderer/Shared/Utility/PipelineStateDescriptionUtility.h>
#include <Renderer/Shared/Utility/CoreRenderResourcesUtility.h>


namespace YAPT
{
	DenoiseStage::DenoiseStage()
	{

	}
	DenoiseStage::~DenoiseStage()
	{

	}

	void DenoiseStage::initialize()
	{

	}
	void DenoiseStage::shutdown()
	{

	}
	void DenoiseStage::onRenderGraphCompiled(const RenderGraphLifetimeData& data)
	{

	}
	void DenoiseStage::onRenderResolutionChanged(const RenderResolutionDependantResourcesData& data)
	{

	}
	void DenoiseStage::prepare(const PrepareData& data)
	{

	}
	void DenoiseStage::update(const UpdateData& data)
	{

	}

	RenderStageConnection DenoiseStage::getOutputConnection(size_t id)
	{
		assert(!"TODO");
		return RenderStageConnection();

	}
	void DenoiseStage::setInputConnection(size_t id, const RenderStageConnection& connection)
	{

	}

	void DenoiseStage::replicateRenderVars()
	{

	}
}