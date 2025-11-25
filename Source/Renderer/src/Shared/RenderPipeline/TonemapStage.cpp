#include <Renderer/Shared/RenderPipeline/TonemapStage.h>
#include <Gfx/RenderGraph/RenderGraph.h>
#include <Renderer/Shared/Utility/RenderResourcesPool.h>
#include <Gfx/GfxApi.h>
#include <Renderer/Shared/CRenderer.h>
#include <Renderer/Shared/Utility/PipelineStateDescriptionUtility.h>
#include <Renderer/Shared/Utility/CoreRenderResourcesUtility.h>

//must agree with shader
#define HISTOGRAM_BUCKETS_COUNT 128
#define GATHER_LUMINANCE_TG_SIZE 16

namespace YAPT
{
	TonemapStage::TonemapStage(bool outputDirectlyToSwapchain)
		:m_outputToSwapchain(outputDirectlyToSwapchain),
		m_tonemapNode(nullptr),
		m_generateHistogramNode(nullptr),
		m_analyzeHistogramNode(nullptr),
		m_colorTarget(YAPT_NULL_HANDLE),
		m_isFirstRun(false)
	{
		                         
	}
	TonemapStage::~TonemapStage()
	{
		 
	}

	void TonemapStage::replicateRenderVars()
	{
		CRendererConfiguration& config = getRenderer()->getConcreteRendererConfiguration();
		float toe = config.getRendererVarValueInternal<float>(RVARNAME_TONEMAP_TOE);
		float mid = config.getRendererVarValueInternal<float>(RVARNAME_TONEMAP_MID);
		float shoulder = config.getRendererVarValueInternal<float>(RVARNAME_TONEMAP_SHOULDER);

		int32_t enableAutoExposure = config.getRendererVarValueInternal<int32_t>(RVARNAME_TONEMAP_USE_AUTOEXPOSURE);
		float manualExposure = config.getRendererVarValueInternal<float>(RVARNAME_TONEMAP_MANUALEXPOSURE);
		float eyeAdaptSpeed = config.getRendererVarValueInternal<float>(RVARNAME_TONEMAP_EYE_ADAPT_SPEED);
		float exposureCompensation = config.getRendererVarValueInternal<float>(RVARNAME_TONEMAP_EXPOSURE_COMPENSATION);
		
		
		float deltaTime = getRenderer()->getFrameDeltaInSeconds();

		m_tonemapConstants.getData()->toeMidShoulderInit = vec4p(toe, mid, shoulder, m_isFirstRun ? 1.0f : 0.f);
		m_tonemapConstants.getData()->exposureEyeAdaptTime = vec4p(enableAutoExposure == 0 ? 0.0f : 1.0f, enableAutoExposure != 0 ? exposureCompensation : manualExposure, eyeAdaptSpeed, deltaTime);
		 
		m_isFirstRun = false;
	}

	void TonemapStage::initialize()
	{
		//nodes
		{
			RenderGraphNodeSlotDefinition slotdefs[] =
			{
				{
				RenderGraphBufferSlotDefinition(RESOURCE_USAGE_STORAGE_BUFFER,
				ACCESS_FLAGS_WRITE,
				SHADERSTAGE_COMPUTE)
				} 
			};

			m_clearHistogramNode = getGraph()->createComputeNode(1, slotdefs, []
			(RenderGraphNode* node, const RenderGraphNodeExecutionContext& execContext, void* usrData)
				{
					static_cast<TonemapStage*>(usrData)->executeClearHistogram(execContext);
				},
				this, "clearHistogramNode");
		}
		{
			RenderGraphNodeSlotDefinition slotdefs[] =
			{
			{RenderGraphBufferSlotDefinition(RESOURCE_USAGE_STORAGE_BUFFER,
				ACCESS_FLAGS_WRITE,
				SHADERSTAGE_COMPUTE)},
			{RenderGraphTextureSlotDefinition(ResourceDimension::TEXTURE_2D,
				ResourceFormat::UNKNOWN,
				RESOURCE_USAGE_SAMPLED_TEXTURE,
				ACCESS_FLAGS_READ,
				SHADERSTAGE_COMPUTE,
				1,
				1)}
			};

			m_generateHistogramNode = getGraph()->createComputeNode(2, slotdefs, []
			(RenderGraphNode* node, const RenderGraphNodeExecutionContext& execContext, void* usrData)
				{
					static_cast<TonemapStage*>(usrData)->executeGenerateHistogram(execContext);
				},
				this, "generateHistogramNode");
		}

		{
			RenderGraphNodeSlotDefinition slotdefs[] =
			{
			{RenderGraphBufferSlotDefinition(
				RESOURCE_USAGE_STORAGE_BUFFER,
				ACCESS_FLAGS_WRITE,
				SHADERSTAGE_COMPUTE
				)},
			{RenderGraphBufferSlotDefinition(
				RESOURCE_USAGE_STORAGE_BUFFER,
				ACCESS_FLAGS_READ,
				SHADERSTAGE_COMPUTE)},
			};
			
			m_analyzeHistogramNode = getGraph()->createComputeNode(2, slotdefs, []
			(RenderGraphNode* node, const RenderGraphNodeExecutionContext& execContext, void* usrData)
				{
					static_cast<TonemapStage*>(usrData)->executeAnalyzeHistogram(execContext);
				},
				this, "analyzeHistogramNode");
		}
		//prepare tonemapping data
		{
			RenderGraphNodeSlotDefinition slotdefs[] =
			{
			{RenderGraphBufferSlotDefinition(
				RESOURCE_USAGE_STORAGE_BUFFER,
				ACCESS_FLAGS_READ_WRITE,
				SHADERSTAGE_COMPUTE)},
			{RenderGraphBufferSlotDefinition(
				RESOURCE_USAGE_STORAGE_BUFFER,
				ACCESS_FLAGS_READ,
				SHADERSTAGE_COMPUTE)},
			};

			m_preparetonemapDataNode = getGraph()->createComputeNode(2, slotdefs, []
			(RenderGraphNode* node, const RenderGraphNodeExecutionContext& execContext, void* usrData)
				{
					static_cast<TonemapStage*>(usrData)->executePrepareTonemapData(execContext);
				},
				this, "prepareTonemapData");
			

		}

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
				1)},
			{RenderGraphBufferSlotDefinition(
				RESOURCE_USAGE_STORAGE_BUFFER,
				ACCESS_FLAGS_READ,
				SHADERSTAGE_FRAGMENT)}
			};

			m_tonemapNode = getGraph()->createRenderNode(3, slotdefs, []
			(RenderGraphNode* node, const RenderGraphNodeExecutionContext& execContext, void* usrData)
				{
					static_cast<TonemapStage*>(usrData)->executeTonemapping(execContext);
				},
				this, "TonemapNode");

		}
		
		getGraph()->createEdge(m_clearHistogramNode, 0, m_generateHistogramNode, 0);
		getGraph()->createEdge(m_generateHistogramNode, 0, m_analyzeHistogramNode, 1);
		getGraph()->createEdge(m_analyzeHistogramNode, 0, m_preparetonemapDataNode, 1);
		getGraph()->createEdge(m_preparetonemapDataNode, 0, m_tonemapNode, 2);
		
		ShaderLoader* loader = getRenderer()->getShaderLoader();
		//<pipeline layouts>
		//clear histogram
		{
			const ShaderLoader::ShaderPipelineInfo* clear = loader->getShaderPipeline("clearLuminanceHistogram");
			m_clearHistogramPass.init(getRenderer(), clear, nullptr, 0);
			m_clearHistogramPass.createPipelineState();
			 

		}
		//generate histogram
		{
			const ShaderLoader::ShaderPipelineInfo* generate = loader->getShaderPipeline("gatherLuminanceHistogram");
			m_generateHistogramPass.init(getRenderer(),  generate, nullptr, 0);
			m_generateHistogramPass.createPipelineState();
		}
		

		//analyze histogram
		{
			const ShaderLoader::ShaderPipelineInfo* analyze = loader->getShaderPipeline("analyzeLuminanceHistogram");
			m_analyzeHistogramPass.init(getRenderer(),  analyze, nullptr, 0);
			m_analyzeHistogramPass.createPipelineState();
		}

		//prepareTonemap data
		{
			const ShaderLoader::ShaderPipelineInfo* prepare = loader->getShaderPipeline("PrepareTonemapData");
			m_preparetonemapDataPass.init(getRenderer(),  prepare, nullptr, 0);
			m_preparetonemapDataPass.createPipelineState();
		}
		
		    
		//tonemap
		{
			const ShaderLoader::ShaderPipelineInfo* aces = loader->getShaderPipeline("tonemapAces");
			StaticSamplerEntry samplers[] = { {"colorSampler", getRenderer()->getCoreResources()->getDefaultSampler(DefaultSamplerType::LINEAR_REPEAT)} };
			m_tonemapPass.init(getRenderer(), m_tonemapNode->getRenderPassHandle(), aces, samplers, countOf(samplers));

		}
		 
		m_isFirstRun = true;
		 
	}
	   
	void TonemapStage::onRenderGraphCompiled(const RenderGraphLifetimeData& data)
	{
		{
			RenderGraphResourceId histogramBufferId = m_generateHistogramNode->getRenderGraphResourceIdForSlot(0);

			BufferDesc histogramBufferDesc(RESOURCE_USAGE_STORAGE_BUFFER, sizeof(uint32_t) * HISTOGRAM_BUCKETS_COUNT);
			m_histogramBuffer = data.renderGraphLifetimeResources->requestBuffer(histogramBufferDesc, "Luminance Histogram Buffer");
			getGraph()->setRenderGraphResourceBuffer(histogramBufferId, m_histogramBuffer);
		}
		
		{
			RenderGraphResourceId histogramResultId = m_analyzeHistogramNode->getRenderGraphResourceIdForSlot(0);

			BufferDesc histogramResultBufferDesc(RESOURCE_USAGE_STORAGE_BUFFER, sizeof(LuminanceHistogramAnalysisResults));
			m_luminanceAnalysisResultsBuffer = data.renderGraphLifetimeResources->requestBuffer(histogramResultBufferDesc, "Luminance Info Buffer");
			getGraph()->setRenderGraphResourceBuffer(histogramResultId, m_luminanceAnalysisResultsBuffer);
		}

		{
			RenderGraphResourceId prepareDataId = m_preparetonemapDataNode->getRenderGraphResourceIdForSlot(0);

			BufferDesc exposureInfoBufferDesc(RESOURCE_USAGE_STORAGE_BUFFER, sizeof(ExposureInfo));
			m_exposureInfo = data.renderGraphLifetimeResources->requestBuffer(exposureInfoBufferDesc, "Exposure Info");
			getGraph()->setRenderGraphResourceBuffer(prepareDataId, m_exposureInfo);
		}

		
		m_tonemapConstants.init(data.renderGraphLifetimeResources);
		
	}

	void TonemapStage::shutdown()
	{
		m_clearHistogramPass.deinit();
		m_generateHistogramPass.deinit();
		m_analyzeHistogramPass.deinit();
		m_preparetonemapDataPass.deinit();
		m_tonemapPass.deinit();
		
		
	}

	


	void TonemapStage::onRenderResolutionChanged(const RenderResolutionDependantResourcesData& data)
	{

		m_renderWidth = data.newRenderResolutionWidth;
		m_renderHeight = data.newRenderResolutionHeight;

		RenderGraphResourceId renderTargetResId = m_tonemapNode->getRenderGraphResourceIdForSlot(0);

		const RenderGraphResourceDescription& desc = getGraph()->getRenderGraphResourceDescription(renderTargetResId);

		
		if (!m_outputToSwapchain)
		{
			TextureDesc textureDesc(desc.resourceDimensions, desc.resourceFormat, desc.resourceUsage, data.newRenderResolutionWidth, data.newRenderResolutionHeight, 1, 1);
			m_colorTarget = data.resolutionDependantResourcesPool->requestTexture(textureDesc, "LDR Color Target");
			getGraph()->setRenderGraphResourceTexture(renderTargetResId, m_colorTarget);
		}
		
		
		{
			GraphicsPipelineStateDescHelper& psoState = m_tonemapPass.getPipelineStateHelper();
			psoState.setDefaults((float)data.newRenderResolutionWidth, (float)data.newRenderResolutionHeight);
			m_tonemapPass.createPso();
		}

	}


	void TonemapStage::prepare(const PrepareData& cntx)
	{

	}
	void TonemapStage::update(const UpdateData& cntx)
	{
		replicateRenderVars();

		//histogram clear
		{
			
			BufferViewDesc bv;
			bv.structureStrideInBytes = sizeof(uint32_t);

			BufferHandle generateHistogramBuffer = getGraph()->getBufferFromNodeSlot(m_generateHistogramNode->getSortedIndex(), 0);
			BufferViewHandle generateHistogramBufferView = Gfx::getBufferView(getRenderer()->getGfxHandle(), generateHistogramBuffer, bv);

			DescriptorSetUpdate updates[] = {
				{2, 0, 1, DescriptorPtr(&generateHistogramBufferView)}
			};
			m_clearHistogramPass.reserveAndUpdateDescriptorSet(0, updates, countOf(updates));

		}
		//histogram generate
		{
			TextureViewHandle colorSource = getGraph()->getTextureViewFromNodeSlot(m_generateHistogramNode->getSortedIndex(), 1);

			BufferViewDesc bv;
			bv.structureStrideInBytes = sizeof(uint32_t);

			BufferHandle generateHistogramBuffer = getGraph()->getBufferFromNodeSlot(m_generateHistogramNode->getSortedIndex(), 0);
			BufferViewHandle generateHistogramBufferView = Gfx::getBufferView(getRenderer()->getGfxHandle(), generateHistogramBuffer, bv);

			DescriptorSetUpdate updates[] = {
				{0, 0, 1, DescriptorPtr(&colorSource)},
				{1, 0, 1, DescriptorPtr(m_tonemapConstants.getViewPtr())},
				{2, 0, 1, DescriptorPtr(&generateHistogramBufferView)}
			};
			m_generateHistogramPass.reserveAndUpdateDescriptorSet(0, updates, countOf(updates));

			histogramConstants* histogramConstants = &m_tonemapConstants.getData()->histogramConstants;
			histogramConstants->resolution = glm::uvec2(m_renderWidth, m_renderHeight);
			histogramConstants->minEV = -12;
			  
		}
		  
		//luminance analyze
		{
			BufferViewDesc histogramBV;
			histogramBV.structureStrideInBytes = sizeof(uint32_t);

			BufferViewDesc resultBV;
			resultBV.structureStrideInBytes = YAPT_BUFFER_WHOLE_RESOURCE;

			BufferHandle analyzeResultsBuffer = getGraph()->getBufferFromNodeSlot(m_analyzeHistogramNode->getSortedIndex(), 0);
			BufferHandle histogramBuffer = getGraph()->getBufferFromNodeSlot(m_analyzeHistogramNode->getSortedIndex(), 1);
			
			BufferViewHandle analyzeResultsBufferView = Gfx::getBufferView(getRenderer()->getGfxHandle(), analyzeResultsBuffer, resultBV);
			BufferViewHandle histogramBufferView = Gfx::getBufferView(getRenderer()->getGfxHandle(), histogramBuffer, histogramBV);
			
			
			DescriptorSetUpdate updates[] = {
				{0, 0, 1,DescriptorPtr(&histogramBufferView)},
				{1, 0, 1,DescriptorPtr(m_tonemapConstants.getViewPtr())},
				{2, 0, 1,DescriptorPtr(&analyzeResultsBufferView)}
			};
			m_analyzeHistogramPass.reserveAndUpdateDescriptorSet(0, updates, countOf(updates));
		}

		//tonemapdata prepare
		{
			BufferViewDesc bv;
			bv.structureStrideInBytes = YAPT_BUFFER_WHOLE_RESOURCE;

			BufferHandle exposureInfoBuffer = getGraph()->getBufferFromNodeSlot(m_preparetonemapDataNode->getSortedIndex(), 0);
			BufferHandle histogramResultsBuffer = getGraph()->getBufferFromNodeSlot(m_preparetonemapDataNode->getSortedIndex(), 1);

			BufferViewHandle exposureInfoBufferView = Gfx::getBufferView(getRenderer()->getGfxHandle(), exposureInfoBuffer, bv);
			BufferViewHandle histogramResultsBufferView = Gfx::getBufferView(getRenderer()->getGfxHandle(), histogramResultsBuffer, bv);

			DescriptorSetUpdate updates[] = {
				{0, 0, 1, DescriptorPtr(&histogramResultsBufferView)},
				{1, 0, 1, DescriptorPtr(m_tonemapConstants.getViewPtr())},
				{2, 0, 1, DescriptorPtr(&exposureInfoBufferView)}
			};
			m_preparetonemapDataPass.reserveAndUpdateDescriptorSet(0, updates, countOf(updates));
		}
		 
		//tonemap
		{

			TextureViewHandle colorSource = getGraph()->getTextureViewFromNodeSlot(m_tonemapNode->getSortedIndex(), 1);
			 
			BufferViewDesc bv;
			bv.structureStrideInBytes = YAPT_BUFFER_WHOLE_RESOURCE;

			BufferHandle exposureInfoSource = getGraph()->getBufferFromNodeSlot(m_tonemapNode->getSortedIndex(), 2);
			BufferViewHandle exposureBufferViewHandle = Gfx::getBufferView(getRenderer()->getGfxHandle(), exposureInfoSource, bv);

			DescriptorSetUpdate updates[] = {
				{0, 0, 1, DescriptorPtr(&colorSource)},
				{2, 0, 1, DescriptorPtr(m_tonemapConstants.getViewPtr())},
				{3, 0, 1, DescriptorPtr(&exposureBufferViewHandle)}
			};
			m_tonemapPass.reserveAndUpdateDescriptorSet(0, updates, countOf(updates));
		}
		

		m_tonemapConstants.flush();
	}

	void TonemapStage::executeClearHistogram(const RenderGraphNodeExecutionContext& execContext)
	{
		m_clearHistogramPass.dispatch(execContext.cmdBuffer, 1, 1, 1);
	}
	
	void TonemapStage::executeGenerateHistogram(const RenderGraphNodeExecutionContext& execContext)
	{
		
		uint32_t x = (m_renderWidth + GATHER_LUMINANCE_TG_SIZE - 1) / GATHER_LUMINANCE_TG_SIZE;
		uint32_t y = (m_renderHeight + GATHER_LUMINANCE_TG_SIZE - 1) / GATHER_LUMINANCE_TG_SIZE;
		m_generateHistogramPass.dispatch(execContext.cmdBuffer, x, y, 1);
	}
	void TonemapStage::executeAnalyzeHistogram(const RenderGraphNodeExecutionContext& execContext)
	{
		m_analyzeHistogramPass.dispatch(execContext.cmdBuffer, 1, 1, 1);
	}

	void TonemapStage::executePrepareTonemapData(const RenderGraphNodeExecutionContext& execContext)
	{
		m_preparetonemapDataPass.dispatch(execContext.cmdBuffer, 1, 1, 1);
	}

	void TonemapStage::executeTonemapping(const RenderGraphNodeExecutionContext& execContext)
	{
		m_tonemapPass.drawFullscreenPass(execContext.cmdBuffer);
	}

	RenderStageConnection TonemapStage::getOutputConnection(size_t id)
	{
		RenderStageConnection conn;
		switch (id)
		{
		case TONEMAP_STAGE_CONNECTION_COLOR:
		{
			conn.node = m_tonemapNode;
			conn.slot = 0;
			break;
		}
		default:
			assert(false && "Stage connection id not recognized");
		}
		return conn;
	}
	void TonemapStage::setInputConnection(size_t id, const RenderStageConnection& connection)
	{
		switch (id)
		{
		case TONEMAP_STAGE_CONNECTION_COLOR:
		{
			getGraph()->createEdge(connection.node, connection.slot, m_generateHistogramNode, 1);
			getGraph()->createEdge(connection.node, connection.slot, m_tonemapNode, 1);
			break;
		}
		default:
			assert(false && "Stage connection id not recognized");
		}
	}

}