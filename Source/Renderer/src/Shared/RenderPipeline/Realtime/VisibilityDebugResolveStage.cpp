#include <Renderer/Shared/RenderPipeline/Realtime/VisibilityDebugResolveStage.h>
#include <Gfx/RenderGraph/RenderGraph.h>
#include <Renderer/Shared/Utility/RenderResourcesPool.h>
#include <Renderer/Shared/CRenderer.h>

namespace YAPT
{
	VisibilityDebugResolveStage::VisibilityDebugResolveStage()
		:m_resolveNode(nullptr),
		m_colorTarget(YAPT_NULL_HANDLE)
	{

	}

	VisibilityDebugResolveStage::~VisibilityDebugResolveStage()
	{

	}

	void VisibilityDebugResolveStage::initialize(const InitializeData& data)
	{
		const ShaderLoader::ShaderPipelineInfo* resolve = getRenderer()->getShaderLoader()->getShaderPipeline("realtimeVisibilityDebugResolve");
		m_resolvePass.init(getRenderer(), resolve, nullptr, 0);
	}

	void VisibilityDebugResolveStage::createNodes()
	{
		RenderGraphNodeSlotDefinition slotdefs[] =
		{
			{RenderGraphTextureSlotDefinition(ResourceDimension::TEXTURE_2D,
				ResourceFormat::UNKNOWN,
				RESOURCE_USAGE_RENDER_TARGET_TEXTURE,
				ACCESS_FLAGS_READ_WRITE,
				SHADERSTAGE_FRAGMENT,
				1,
				1)},
			{RenderGraphTextureSlotDefinition(ResourceDimension::TEXTURE_2D,
				ResourceFormat::UNKNOWN,
				RESOURCE_USAGE_SAMPLED_TEXTURE,
				ACCESS_FLAGS_READ,
				SHADERSTAGE_FRAGMENT,
				1,
				1)}
		};

		m_resolveNode = getGraph()->createRenderNode(2, slotdefs, []
		(RenderGraphNode* node, const RenderGraphNodeExecutionContext& execContext, void* usrData)
			{
				static_cast<VisibilityDebugResolveStage*>(usrData)->execute(execContext);
			},
			this, "visibilityDebugResolve");
	}

	void VisibilityDebugResolveStage::onRenderGraphCompiled(const RenderGraphLifetimeData& data)
	{
		m_resolvePass.setRenderPass(m_resolveNode);
	}

	void VisibilityDebugResolveStage::shutdown()
	{
		m_resolvePass.deinit();
	}

	void VisibilityDebugResolveStage::onRenderResolutionChanged(const RenderResolutionDependantResourcesData& data)
	{
		RenderGraphResourceId renderTargetResId = m_resolveNode->getRenderGraphResourceIdForSlot(0);
		const RenderGraphResourceDescription& desc = getGraph()->getRenderGraphResourceDescription(renderTargetResId);

		TextureDesc textureDesc(desc.resourceDimensions, desc.resourceFormat, desc.resourceUsage, data.newRenderResolutionWidth, data.newRenderResolutionHeight, 1, 1);
		m_colorTarget = data.resolutionDependantResourcesPool->requestTexture(textureDesc, "Visibility Debug Resolve Target");
		getGraph()->setRenderGraphResourceTexture(renderTargetResId, m_colorTarget);

		if (data.resolutionChanged)
		{
			GraphicsPipelineStateDescHelper& psoState = m_resolvePass.getPipelineStateHelper();
			psoState.setDefaults((float)data.newRenderResolutionWidth, (float)data.newRenderResolutionHeight);
			m_resolvePass.createPso();
		}
		else
		{
			m_resolvePass.createPsoIfRenderPassChanged();
		}
	}

	JobHandle VisibilityDebugResolveStage::prepare(const PrepareData& data)
	{
		return {};
	}

	JobHandle VisibilityDebugResolveStage::update(const UpdateData& data)
	{
		TextureViewHandle visibility = getGraph()->getTextureViewFromNodeSlot(m_resolveNode->getSortedIndex(), 1);

		DescriptorSetUpdate updates[] = {
			{0, 0, 1, DescriptorPtr(&visibility)}
		};
		m_resolvePass.reserveAndUpdateDescriptorSet(0, updates, countOf(updates));
		return {};
	}

	void VisibilityDebugResolveStage::execute(const RenderGraphNodeExecutionContext& execContext)
	{
		m_resolvePass.drawFullscreenPass(execContext.cmdBuffer);
	}

	RenderStageConnection VisibilityDebugResolveStage::getOutputConnection(size_t id)
	{
		RenderStageConnection conn;
		switch (id)
		{
		case VISIBILITYRESOLVE_STAGE_CONNECTION_COLOR:
			conn.node = m_resolveNode;
			conn.slot = 0;
			break;
		default:
			assert(false && "Stage connection id not recognized");
		}
		return conn;
	}

	void VisibilityDebugResolveStage::setInputConnection(size_t id, const RenderStageConnection& connection)
	{
		switch (id)
		{
		case VISIBILITYRESOLVE_STAGE_CONNECTION_VISIBILITY:
			getGraph()->createEdge(connection.node, connection.slot, m_resolveNode, 1);
			break;
		default:
			assert(false && "Stage connection id not recognized");
		}
	}
}
