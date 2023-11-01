#include <Renderer/Shared/RenderPipeline/MeshDebugStage.h>
#include <Renderer/Shared/RenderGraph/RenderGraph.h>
#include <Renderer/Shared/Utility/RenderResourcesPool.h>
#include <Renderer/Shared/GfxApi.h>
#include <Renderer/Shared/CRenderer.h>
#include <Renderer/Shared/Utility/PipelineStateDescriptionUtility.h>
#include <Renderer/Shared/Utility/CoreRenderResourcesUtility.h>

namespace YAPT
{
	MeshDebugStage::MeshDebugStage(bool outputDirectlyToSwapchain)
		:m_outputToSwapchain(outputDirectlyToSwapchain),
		m_renderMeshesNode(nullptr),
		m_colorTarget(YAPT_NULL_HANDLE),
		m_descSet(YAPT_NULL_HANDLE)
	{
		  
	}
	MeshDebugStage::~MeshDebugStage()
	{

	}

	void MeshDebugStage::replicateRenderVars()
	{

	}

	void MeshDebugStage::initialize()
	{
		
		{
			RenderGraphNodeSlotDefinition slotdefs[] =
			{
				{
					RenderGraphTextureSlotDefinition(ResourceDimension::TEXTURE_2D,
					ResourceFormat::RGBA8_SRGB,
					RESOURCE_USAGE_RENDER_TARGET_TEXTURE,
					ACCESS_FLAGS_READ_WRITE,
					SHADERSTAGE_FRAGMENT,
					1,
					1,
					RenderNodeClearFrequency::ALWAYS,
					ClearValue::ClearValue(0.05f, 0.05f, 0.05f, 1.f))
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
					ClearValue::ClearValue(1.0f, 0))
				}
			};

			m_renderMeshesNode = getGraph()->createRenderNode(2, slotdefs, []
			(RenderGraphNode* node, const RenderGraphNodeExecutionContext& execContext, void* usrData)
				{
					static_cast<MeshDebugStage*>(usrData)->execute(execContext);
				},
				this, "meshDebugNode");

		}

		
		m_renderDataIndex = getRenderer()->getRenderObjectManager().acquireRenderDataIndex();
		assert(getRenderer()->getRenderObjectManager().isValidRenderDataIndex(m_renderDataIndex));

		
		m_descSet = YAPT_NULL_HANDLE;

		m_refreshAllRenderObjects = true;
	}

	void MeshDebugStage::onRenderGraphCompiled(const RenderGraphLifetimeData& data)
	{


		ShaderLoader* loader = getRenderer()->getShaderLoader();
		const ShaderLoader::ShaderPipelineInfo* dbg = loader->getShaderPipeline("meshDebug");
		 
		m_vertexInputSemantics.assign(dbg->reflection->getInputAttributes(), dbg->reflection->getInputAttributes() + dbg->reflection->getNumberOfInputAttributes());

		m_layout.initFromShaderReflection(getRenderer()->getGfxHandle(), dbg->reflection);
		m_layout.setDynamic(0, 0);
		m_layout.compile();
		m_psoDescHelper.setupShaderStages(dbg);

		m_psoDescHelper.pipelineDesc.renderPass = m_renderMeshesNode->getRenderPassHandle();
		m_psoDescHelper.pipelineDesc.pipelineLayout = m_layout.getPipelineLayoutHandle();

	}

	void MeshDebugStage::shutdown()
	{
		clearPSOs();
		getRenderer()->getRenderObjectManager().freeRenderDataIndex(m_renderDataIndex);
		
	}

	void MeshDebugStage::clearPSOs()
	{
		for (auto pso : m_pipelineStateObjects)
		{
			Gfx::destroyGraphicsPipelineState(getRenderer()->getGfxHandle(), pso.pso);
		}
		m_pipelineStateObjects.clear();
		m_meshLayoutToPSOIndex.clear();
	}


	void MeshDebugStage::onRenderResolutionChanged(const RenderResolutionDependantResourcesData& data)
	{

		m_renderWidth = data.newRenderResolutionWidth;
		m_renderHeight = data.newRenderResolutionHeight;

		bool needToCreateColorTarget = !m_outputToSwapchain || (m_renderMeshesNode->getNumberOfInputEdges(0) == 0);
		bool needToCreateDepthTarget = !m_outputToSwapchain || (m_renderMeshesNode->getNumberOfInputEdges(1) == 0);

		if (needToCreateColorTarget)
		{
			//color
			
			RenderGraphResourceId renderTargetResId = m_renderMeshesNode->getRenderGraphResourceIdForSlot(0);
			const RenderGraphResourceDescription& desc = getGraph()->getRenderGraphResourceDescription(renderTargetResId);
			TextureDesc textureDesc(desc.resourceDimensions, desc.resourceFormat, desc.resourceUsage, data.newRenderResolutionWidth, data.newRenderResolutionHeight, 1, 1);
			m_colorTarget = data.resolutionDependantResourcesPool->requestTexture(textureDesc, "Mesh Debug Stage Color Target");
			m_bindingsUtility.setDeferredRenderGraphResourceBinding(renderTargetResId, m_colorTarget);
		}

		//depth
		if(needToCreateDepthTarget)
		{
			RenderGraphResourceId renderTargetResId = m_renderMeshesNode->getRenderGraphResourceIdForSlot(1);
			const RenderGraphResourceDescription& desc = getGraph()->getRenderGraphResourceDescription(renderTargetResId);
			TextureDesc depthDesc(desc.resourceDimensions, desc.resourceFormat, desc.resourceUsage, data.newRenderResolutionWidth, data.newRenderResolutionHeight, 1, 1);
			m_depthTarget = data.resolutionDependantResourcesPool->requestTexture(depthDesc, "Mesh Debug Stage Depth Target");
			m_bindingsUtility.setDeferredRenderGraphResourceBinding(renderTargetResId, m_depthTarget);
		}


		{
			clearPSOs();
			m_refreshAllRenderObjects = true;
			m_psoDescHelper.setDefaults((float)data.newRenderResolutionWidth, (float)data.newRenderResolutionHeight);
			m_psoDescHelper.depthStencilState.depthTestEnable = true;
			m_psoDescHelper.depthStencilState.depthWriteEnable = true;
		}

	}


	void MeshDebugStage::prepare(const PrepareData& cntx)
	{
		m_bindingsUtility.flushDeferredRenderGraphResourceBindings(getGraph());



	}
	void MeshDebugStage::update(const UpdateData& cntx)
	{
		replicateRenderVars();
		
		
		//create descset for this frame
		if (m_descSet != YAPT_NULL_HANDLE)
		{
			m_layout.getDescriptorSetUtility(0).freeDescriptorSet(m_descSet);
		}
		m_descSet = m_layout.getDescriptorSetUtility(0).getNewDescriptorSet();

		BufferViewHandle viewHandle = getRenderer()->getCurrentRenderView().getMVPBufferView();

		DescriptorSetUpdate update;
		update.buffHandles = &viewHandle;
		update.descriptorCount = 1;
		update.dstArrayElement = 0;
		update.dstBinding = 0;

		Gfx::updateDescriptorSet(getRenderer()->getGfxHandle(), m_descSet, &update, 1);

	}

	void MeshDebugStage::execute(const RenderGraphNodeExecutionContext& execContext)
	{
		updateRenderObjectChanges();
		sortRenderObjectsToBuckets();

		//draw
		RenderObjectManager& roMngr = getRenderer()->getRenderObjectManager();
		MeshInternal** meshes = roMngr.getAllMeshes();

		GfxApiHandle gfx = getRenderer()->getGfxHandle();
		std::vector<BufferViewHandle> buffers;

		DescriptorSetHandle descSetBind = m_descSet;

		size_t perObjectBufferEntrySize = getRenderer()->getCurrentRenderView().getMVPBufferPerEntrySize();

		for (size_t b = 0; b < m_renderObjectBuckets.size(); ++b)
		{
			std::vector<size_t>& drawIndices =  m_renderObjectBuckets[b];
			Gfx::setGraphicsPipelineState(gfx, execContext.cmdBuffer, m_pipelineStateObjects[b].pso);
			const MeshToVertexInputLayoutMappingUtility& meshMappingUtility = m_pipelineStateObjects[b].vertexBufferMapping;
			MeshInternal* lastMesh = nullptr;

			for (size_t i = 0; i < drawIndices.size(); ++i)
			{
				
				size_t drawIndex = drawIndices[i];
				MeshInternal* mesh = meshes[drawIndex];
				if (mesh != lastMesh)
				{
					buffers.clear();
					for (size_t vbuff = 0; vbuff < mesh->getLayoutInfo().vertexBufferConfigurations.size(); ++vbuff)
					{
						assert(vbuff < meshMappingUtility.getNumberOfVertexBufferDefinitions());
						if (meshMappingUtility.getVertexBufferDefinitions()[vbuff].numberOfVertexAttributes > 0)
						{
							buffers.push_back(mesh->getVertexBuffer(vbuff).bufferView);
						}
					}

					Gfx::setVertexBuffers(gfx, execContext.cmdBuffer, buffers.data(), buffers.size(), 0);
					Gfx::setIndexBuffer(gfx, execContext.cmdBuffer, mesh->getIndexBuffer().bufferView);
					lastMesh = mesh;
				}
				    
				size_t dynBuffOffset = drawIndex * perObjectBufferEntrySize;
				Gfx::bindDescriptorSets(gfx, execContext.cmdBuffer,BindingPoint::BINDING_POINT_GRAPHICS, m_layout.getPipelineLayoutHandle(), &descSetBind, 0, 1, &dynBuffOffset, 1);

				Gfx::drawIndexed(gfx, execContext.cmdBuffer, uint32_t(mesh->getPrimitiveCount()) * 3, 1, 0, 0, 0);

			}
    
		}

	}

	void MeshDebugStage::sortRenderObjectsToBuckets()
	{
		//sort renderobjects to buckets
		m_renderObjectBuckets.resize(m_pipelineStateObjects.size());
		for (size_t i = 0; i < m_renderObjectBuckets.size(); ++i)
		{
			m_renderObjectBuckets[i].clear();
		}

		RenderObjectManager& roMngr = getRenderer()->getRenderObjectManager();
		RenderObjectManager::RenderData* datas = roMngr.getAllRenderData(m_renderDataIndex);
		for (size_t i = 0; i < roMngr.getNumberOfObjects(); ++i)
		{
			size_t bucketIndex = datas[i].index;
			m_renderObjectBuckets[bucketIndex].push_back(i);

		}
	}

	void MeshDebugStage::updateRenderObjectChanges()
	{
		RenderObjectManager& roMngr = getRenderer()->getRenderObjectManager();
		if (m_refreshAllRenderObjects)
		{
			m_refreshAllRenderObjects = false;
			MeshInternal** meshes = roMngr.getAllMeshes();
			RenderObjectManager::RenderData* datas = roMngr.getAllRenderData(m_renderDataIndex);

			for (size_t i = 0; i < roMngr.getNumberOfObjects(); ++i)
			{
				MeshInternal* mesh = meshes[i];
				RenderObjectManager::RenderData& data = datas[i];
				assignToPSOBucket(mesh, data);
			}
		}
		else
		{
			const RenderObjectId* ids;
			size_t count;
			roMngr.getCreatedEntries(ids, count);
			for (size_t i = 0; i < count; ++i)
			{
				assignToPSOBucket(roMngr.getMeshForId(ids[i]), *roMngr.getRenderDataForId(ids[i],m_renderDataIndex));
			}

			roMngr.getModifiedEntries(ids, count);
			for (size_t i = 0; i < count; ++i)
			{
				assignToPSOBucket(roMngr.getMeshForId(ids[i]), *roMngr.getRenderDataForId(ids[i], m_renderDataIndex));
			}
		}
		
	}

	void MeshDebugStage::assignToPSOBucket(MeshInternal* mesh, RenderObjectManager::RenderData& data)
	{
		auto iter = m_meshLayoutToPSOIndex.find(mesh->getMeshLayoutID());

		if (iter != m_meshLayoutToPSOIndex.end())
		{
			data.index = iter->second;
		}
		else
		{			
			m_pipelineStateObjects.resize(m_pipelineStateObjects.size() + 1);
			PSOEntry& psoEntry = m_pipelineStateObjects.back();
			bool mappingSuccesfull = psoEntry.vertexBufferMapping.generateMapping(mesh->getLayoutInfo(), m_vertexInputSemantics.data(), m_vertexInputSemantics.size());
			assert(mappingSuccesfull);

			m_psoDescHelper.setVertexBufferDefinitions(psoEntry.vertexBufferMapping.getVertexBufferDefinitions(), psoEntry.vertexBufferMapping.getNumberOfVertexBufferDefinitions());
			psoEntry.pso = Gfx::createGraphicsPipelineState(getRenderer()->getGfxHandle(), m_psoDescHelper.pipelineDesc);
			m_psoDescHelper.setVertexBufferDefinitions(nullptr, 0);

			m_meshLayoutToPSOIndex[mesh->getMeshLayoutID()] = m_pipelineStateObjects.size() - 1;
			

		}
		
	}

	RenderStageConnection MeshDebugStage::getOutputConnection(size_t id)
	{
		RenderStageConnection conn;
		switch (id)
		{
		case MESHDEBUG_STAGE_CONNECTION_COLOR:
		{
			conn.node = m_renderMeshesNode;
			conn.slot = 0;
			break;
		}

		case MESHDEBUG_STAGE_CONNECTION_DEPTH:
			conn.node = m_renderMeshesNode;
			conn.slot = 1;
			break;

		default:
			assert(false && "Stage connection id not recognized");
		}
		return conn;
	}
	void MeshDebugStage::setInputConnection(size_t id, const RenderStageConnection& connection)
	{
		switch (id)
		{
		case MESHDEBUG_STAGE_CONNECTION_COLOR:
		{
			getGraph()->createEdge(connection.node, connection.slot, m_renderMeshesNode, 0);
			break;
		}
		case MESHDEBUG_STAGE_CONNECTION_DEPTH:
			getGraph()->createEdge(connection.node, connection.slot, m_renderMeshesNode, 1);
			break;

		default:
			assert(false && "Stage connection id not recognized");
		}
	}

}