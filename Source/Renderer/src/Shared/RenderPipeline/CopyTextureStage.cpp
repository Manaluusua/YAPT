#include <Renderer/Shared/RenderPipeline/CopyTextureStage.h>
#include <Gfx/RenderGraph/RenderGraph.h>
#include <Renderer/Shared/Utility/RenderResourcesPool.h>
#include <Gfx/GfxApi.h>
#include <Renderer/Shared/CRenderer.h>
#include <Renderer/Shared/Utility/PipelineStateDescriptionUtility.h>
#include <Renderer/Shared/Utility/CoreRenderResourcesUtility.h>

namespace YAPT
{
	CopyTextureStage::CopyTextureStage(ResourceFormat forceDestinationFormat, ResourceFormat forceSourceformat)
		:m_copyNode(nullptr),
		m_destinationTexture(YAPT_NULL_HANDLE),
		m_destinationFormat(forceDestinationFormat),
		m_sourceFormat(forceSourceformat),
		m_renderWidth(0),
		m_renderHeight(0)
	{

	}
	CopyTextureStage::~CopyTextureStage()
	{

	}

	void CopyTextureStage::initialize()
	{
		{
			RenderGraphNodeSlotDefinition slotdefs[] =
			{
				{RenderGraphTextureSlotDefinition(ResourceDimension::TEXTURE_2D,
					m_destinationFormat,
					RESOURCE_USAGE_RENDER_TARGET_TEXTURE,
					ACCESS_FLAGS_READ_WRITE,
					SHADERSTAGE_FRAGMENT,
					1,
					1)},
				{RenderGraphTextureSlotDefinition(ResourceDimension::TEXTURE_2D,
					m_sourceFormat,
					RESOURCE_USAGE_SAMPLED_TEXTURE,
					ACCESS_FLAGS_READ,
					SHADERSTAGE_FRAGMENT,
					1,
					1)}
			};

			m_copyNode = getGraph()->createRenderNode(countOf(slotdefs), slotdefs, []
			(RenderGraphNode* node, const RenderGraphNodeExecutionContext& execContext, void* usrData)
				{
					static_cast<CopyTextureStage*>(usrData)->executeCopy(execContext);
				},
				this, "copyTextureNode");
		}

		ShaderLoader* loader = getRenderer()->getShaderLoader();
		const ShaderLoader::ShaderPipelineInfo* copy = loader->getShaderPipeline("copyTexture");
		StaticSamplerEntry samplers[] = { {"colorSampler", getRenderer()->getCoreResources()->getDefaultSampler(DefaultSamplerType::LINEAR_REPEAT)} };
		m_copyPass.init(getRenderer(), m_copyNode->getRenderPassHandle(), copy, samplers, countOf(samplers));
	}

	void CopyTextureStage::shutdown()
	{
		m_copyPass.deinit();
	}

	void CopyTextureStage::onRenderResolutionChanged(const RenderResolutionDependantResourcesData& data)
	{
		m_renderWidth = data.newRenderResolutionWidth;
		m_renderHeight = data.newRenderResolutionHeight;

		if (m_copyNode->getNumberOfInputEdges(0) == 0)
		{
			RenderGraphResourceId destinationResId = m_copyNode->getRenderGraphResourceIdForSlot(0);
			const RenderGraphResourceDescription& desc = getGraph()->getRenderGraphResourceDescription(destinationResId);

			TextureDesc textureDesc(desc.resourceDimensions, desc.resourceFormat, desc.resourceUsage, m_renderWidth, m_renderHeight, 1, 1);
			m_destinationTexture = data.resolutionDependantResourcesPool->requestTexture(textureDesc);
			getGraph()->setRenderGraphResourceTexture(destinationResId, m_destinationTexture);
		}

		GraphicsPipelineStateDescHelper& psoState = m_copyPass.getPipelineStateHelper();
		psoState.setDefaults((float)m_renderWidth, (float)m_renderHeight);
		m_copyPass.createPso();
	}

	JobHandle CopyTextureStage::prepare(const PrepareData& cntx)
	{
		return {};
	}
	JobHandle CopyTextureStage::update(const UpdateData& cntx)
	{
		TextureViewHandle source = getGraph()->getTextureViewFromNodeSlot(m_copyNode->getSortedIndex(), 1);

		DescriptorSetUpdate updates[] = {
			{0, 0, 1, DescriptorPtr(&source)}
		};
		m_copyPass.reserveAndUpdateDescriptorSet(0, updates, countOf(updates));
		return {};
	}

	void CopyTextureStage::executeCopy(const RenderGraphNodeExecutionContext& execContext)
	{
		m_copyPass.drawFullscreenPass(execContext.cmdBuffer);
	}

	RenderStageConnection CopyTextureStage::getOutputConnection(size_t id)
	{
		RenderStageConnection conn;
		switch (id)
		{
		case COPYTEXTURE_STAGE_CONNECTION_DESTINATION:
		{
			conn.node = m_copyNode;
			conn.slot = 0;
			break;
		}
		default:
			assert(false && "Stage connection id not recognized");
		}
		return conn;
	}
	void CopyTextureStage::setInputConnection(size_t id, const RenderStageConnection& connection)
	{
		switch (id)
		{
		case COPYTEXTURE_STAGE_CONNECTION_SOURCE:
		{
			getGraph()->createEdge(connection.node, connection.slot, m_copyNode, 1);
			break;
		}
		case COPYTEXTURE_STAGE_CONNECTION_DESTINATION:
		{
			getGraph()->createEdge(connection.node, connection.slot, m_copyNode, 0);
			break;
		}
		default:
			assert(false && "Stage connection id not recognized");
		}
	}
}
