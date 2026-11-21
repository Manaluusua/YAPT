#include <Renderer/Shared/RenderPipeline/Realtime/DeferredMaterialsStage.h>
#include <Renderer/Shared/RenderPipeline/Realtime/RealtimeSceneResources.h>
#include <Gfx/RenderGraph/RenderGraph.h>
#include <Renderer/Shared/Utility/RenderResourcesPool.h>
#include <Renderer/Shared/CRenderer.h>
#include <Renderer/Shared/BindlessTextureManager.h>
#include <Renderer/Shared/BindlessBufferManager.h>
#include <Renderer/Shared/BindlessMeshManager.h>
#include <Renderer/Shared/BindlessMaterialManager.h>

namespace YAPT
{
	static const ClearValue s_VisibilityClear = ClearValue(0.f, 0.f, 0.f, 0.f);
	static const ClearValue s_DepthClear = ClearValue(1.0f, 0);

	DeferredMaterialsStage::DeferredMaterialsStage(RealtimeSceneResources* sceneResources)
		:m_sceneResources(sceneResources),
		m_renderNode(nullptr),
		m_visibilityTarget(YAPT_NULL_HANDLE),
		m_depthTarget(YAPT_NULL_HANDLE),
		m_pso(YAPT_NULL_HANDLE),
		m_descSet(YAPT_NULL_HANDLE),
		m_psoRenderPassSignature(0)
	{

	}

	DeferredMaterialsStage::~DeferredMaterialsStage()
	{

	}

	void DeferredMaterialsStage::initialize(const InitializeData& data)
	{
		ShaderLoader* loader = getRenderer()->getShaderLoader();
		const ShaderLoader::ShaderPipelineInfo* pipelineInfo = loader->getShaderPipeline("realtimeDeferredMaterials");

		m_layout.initFromShaderReflection(getRenderer()->getGfxHandle(), pipelineInfo->reflection);
		{
			BindlessTextureManager* texMngr = getRenderer()->getTextureManager();
			BindlessBufferManager* buffMngr = getRenderer()->getBufferManager();
			const DescriptorSetLayoutBinding& bindingTex = texMngr->getBindingDefinition();
			const DescriptorSetLayoutBinding& bindingBuff = buffMngr->getBindingDefinition();
			m_layout.setExplicitDescriptorSetLayout(1, &bindingTex, 1, texMngr->getLayout());
			m_layout.setExplicitDescriptorSetLayout(2, &bindingBuff, 1, buffMngr->getLayout());
		}
		m_layout.compile();

		m_psoDescHelper.setupShaderStages(pipelineInfo);
		m_psoDescHelper.pipelineDesc.pipelineLayout = m_layout.getPipelineLayoutHandle();

		m_instanceAttribute.shaderInputSlot = AttributeSemantic(AttributeSemanticName::TEXCOORD, 0);
		m_instanceAttribute.format = ResourceFormat::R32_UINT;
		m_instanceAttribute.perVertexOffset = 0;

		m_instanceBufferDefinition.attributes = &m_instanceAttribute;
		m_instanceBufferDefinition.numberOfVertexAttributes = 1;
		m_instanceBufferDefinition.stride = sizeof(uint32_t);
		m_instanceBufferDefinition.vertexInputRate = 1;

		m_descSet = YAPT_NULL_HANDLE;
		m_pso = YAPT_NULL_HANDLE;
		m_psoRenderPassSignature = 0;
	}

	void DeferredMaterialsStage::createNodes()
	{
		RenderGraphNodeSlotDefinition slotdefs[] =
		{
			{
				RenderGraphTextureSlotDefinition(ResourceDimension::TEXTURE_2D,
				VisibilityFormat,
				RESOURCE_USAGE_RENDER_TARGET_TEXTURE,
				ACCESS_FLAGS_READ_WRITE,
				SHADERSTAGE_FRAGMENT,
				1,
				1,
				RenderNodeClearFrequency::ALWAYS,
				s_VisibilityClear)
			},
			{
				RenderGraphTextureSlotDefinition(ResourceDimension::TEXTURE_2D,
				ResourceFormat::D24_UNORM_S8_UINT,
				RESOURCE_USAGE_DEPTH_STENCIL_TEXTURE,
				ACCESS_FLAGS_READ_WRITE,
				SHADERSTAGE_FRAGMENT,
				1,
				1,
				RenderNodeClearFrequency::ALWAYS,
				s_DepthClear)
			}
		};

		m_renderNode = getGraph()->createRenderNode(2, slotdefs, []
		(RenderGraphNode* node, const RenderGraphNodeExecutionContext& execContext, void* usrData)
			{
				static_cast<DeferredMaterialsStage*>(usrData)->execute(execContext);
			},
			this, "deferredMaterialsNode");
	}

	void DeferredMaterialsStage::onRenderGraphCompiled(const RenderGraphLifetimeData& data)
	{
		m_psoDescHelper.pipelineDesc.renderPass = m_renderNode->getRenderPassHandle();

		uint64_t signature = m_renderNode->getAttachmentSignature();
		if (signature != m_psoRenderPassSignature)
		{
			destroyPso();
			m_psoRenderPassSignature = signature;
		}
	}

	void DeferredMaterialsStage::shutdown()
	{
		destroyPso();
		if (m_descSet != YAPT_NULL_HANDLE)
		{
			m_layout.getDescriptorSetUtility(0).freeDescriptorSet(m_descSet);
			m_descSet = YAPT_NULL_HANDLE;
		}
	}

	void DeferredMaterialsStage::destroyPso()
	{
		if (m_pso != YAPT_NULL_HANDLE)
		{
			Gfx::destroyGraphicsPipelineState(getRenderer()->getGfxHandle(), m_pso);
			m_pso = YAPT_NULL_HANDLE;
		}
	}

	void DeferredMaterialsStage::recreatePso()
	{
		destroyPso();
		m_psoDescHelper.setVertexBufferDefinitions(&m_instanceBufferDefinition, 1);
		m_pso = Gfx::createGraphicsPipelineState(getRenderer()->getGfxHandle(), m_psoDescHelper.pipelineDesc);
		m_psoDescHelper.setVertexBufferDefinitions(nullptr, 0);
	}

	void DeferredMaterialsStage::onRenderResolutionChanged(const RenderResolutionDependantResourcesData& data)
	{
		{
			RenderGraphResourceId resId = m_renderNode->getRenderGraphResourceIdForSlot(0);
			const RenderGraphResourceDescription& desc = getGraph()->getRenderGraphResourceDescription(resId);
			TextureDesc textureDesc(desc.resourceDimensions, desc.resourceFormat, desc.resourceUsage, data.newRenderResolutionWidth, data.newRenderResolutionHeight, 1, 1, MemoryType::DEFAULT, s_VisibilityClear, true);
			m_visibilityTarget = data.resolutionDependantResourcesPool->requestTexture(textureDesc, "Deferred Materials Visibility Target");
			getGraph()->setRenderGraphResourceTexture(resId, m_visibilityTarget);
		}

		{
			RenderGraphResourceId resId = m_renderNode->getRenderGraphResourceIdForSlot(1);
			const RenderGraphResourceDescription& desc = getGraph()->getRenderGraphResourceDescription(resId);
			TextureDesc depthDesc(desc.resourceDimensions, desc.resourceFormat, desc.resourceUsage, data.newRenderResolutionWidth, data.newRenderResolutionHeight, 1, 1, MemoryType::DEFAULT, s_DepthClear, true);
			m_depthTarget = data.resolutionDependantResourcesPool->requestTexture(depthDesc, "Deferred Materials Depth Target");
			getGraph()->setRenderGraphResourceTexture(resId, m_depthTarget);
		}

		if (data.resolutionChanged)
		{
			m_psoDescHelper.setDefaults((float)data.newRenderResolutionWidth, (float)data.newRenderResolutionHeight);
			m_psoDescHelper.depthStencilState.depthTestEnable = true;
			m_psoDescHelper.depthStencilState.depthWriteEnable = true;
			recreatePso();
		}
		else if (m_pso == YAPT_NULL_HANDLE)
		{
			recreatePso();
		}
	}

	JobHandle DeferredMaterialsStage::prepare(const PrepareData& data)
	{
		return {};
	}

	JobHandle DeferredMaterialsStage::update(const UpdateData& data)
	{
		DescriptorSetUtility& descSetUtility = m_layout.getDescriptorSetUtility(0);
		if (m_descSet != YAPT_NULL_HANDLE)
		{
			descSetUtility.freeDescriptorSet(m_descSet);
		}
		m_descSet = descSetUtility.getNewDescriptorSet();

		const SceneInstanceData& instanceData = m_sceneResources->getInstanceData();
		BufferViewHandle viewConstants = m_sceneResources->getViewConstantsView();
		BufferViewHandle transforms = instanceData.getTransformsBufferView();
		BufferViewHandle matMeshIndices = instanceData.getMaterialAndMeshIndicesBufferView();
		BufferViewHandle meshEntries = m_sceneResources->getMeshManager()->getBufferViewHandle();

		DescriptorSetUpdate updates[] = {
			{0, 0, 1, DescriptorPtr(&viewConstants)},
			{3, 0, 1, DescriptorPtr(&transforms)},
			{4, 0, 1, DescriptorPtr(&matMeshIndices)},
			{7, 0, 1, DescriptorPtr(&meshEntries)},
		};
		Gfx::updateDescriptorSet(getRenderer()->getGfxHandle(), m_descSet, updates, countOf(updates));
		return {};
	}

	void DeferredMaterialsStage::execute(const RenderGraphNodeExecutionContext& execContext)
	{
		uint32_t drawCount = m_sceneResources->getDrawCount();
		if (drawCount == 0 || m_pso == YAPT_NULL_HANDLE) return;

		GfxApiHandle gfx = getRenderer()->getGfxHandle();
		BindlessTextureManager* texMngr = getRenderer()->getTextureManager();
		BindlessBufferManager* buffMngr = getRenderer()->getBufferManager();

		Gfx::setGraphicsPipelineState(gfx, execContext.cmdBuffer, m_pso);

		DescriptorSetHandle bindings[] = {
			m_descSet,
			texMngr->getTextureArrayDescSet(),
			buffMngr->getBufferArrayDescSet()
		};
		Gfx::bindDescriptorSets(gfx, execContext.cmdBuffer, BindingPoint::BINDING_POINT_GRAPHICS, m_layout.getPipelineLayoutHandle(), bindings, 0, countOf(bindings), nullptr, 0);

		BufferViewHandle instanceBuffer = m_sceneResources->getDrawInstanceIndicesView();
		Gfx::setVertexBuffers(gfx, execContext.cmdBuffer, &instanceBuffer, 1, 0);

		Gfx::drawIndirect(gfx, execContext.cmdBuffer, m_sceneResources->getDrawArgumentsBuffer(), 0, drawCount);
	}

	RenderStageConnection DeferredMaterialsStage::getOutputConnection(size_t id)
	{
		RenderStageConnection conn;
		switch (id)
		{
		case DEFERREDMATERIALS_STAGE_CONNECTION_VISIBILITY:
			conn.node = m_renderNode;
			conn.slot = 0;
			break;
		case DEFERREDMATERIALS_STAGE_CONNECTION_DEPTH:
			conn.node = m_renderNode;
			conn.slot = 1;
			break;
		default:
			assert(false && "Stage connection id not recognized");
		}
		return conn;
	}

	void DeferredMaterialsStage::setInputConnection(size_t id, const RenderStageConnection& connection)
	{
		assert(false && "DeferredMaterialsStage accepts nothing as input!");
	}
}
