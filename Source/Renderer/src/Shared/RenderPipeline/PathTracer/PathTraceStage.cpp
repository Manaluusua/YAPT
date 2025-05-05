#include <Renderer/Shared/RenderPipeline/PathTracer/PathTraceStage.h>
#include <Gfx/RenderGraph/RenderGraph.h>
#include <Renderer/Shared/Utility/RenderResourcesPool.h>
#include <Renderer/Shared/Utility/PipelineStateDescriptionUtility.h>
#include <Renderer/Shared/CRenderer.h>
#include <Renderer/Shared/Utility/CoreRenderResourcesUtility.h>
#include <Renderer/Shared/Utility/SpectralUtility.h>
#include <Renderer/Shared/Utility/RenderAPIAbstractionUtility.h>
#include <Renderer/Shared/BindlessTextureManager.h>
#include <Renderer/Shared/BindlessBufferManager.h>
#include <Math/RandUtility.h>
#include <Math/MathUtility.h>
#include <Gfx/GfxBasicTypesUtility.h>

#include <Renderer/Shared/TextureImpl.h>
#include <Renderer/Shared/BufferImpl.h>



using namespace YAPT::MathUtils;

namespace YAPT
{


	PathTraceStage::PathTraceStage(BindlessMaterialManager* matMngr, BindlessMeshManager* meshMngr)
		:m_matMngr(matMngr),
		m_meshMngr(meshMngr),
		m_resolveTargetWidth(0),
		m_resolveTargetHeight(0),
		m_raysPerFrameDivisor(1),
		m_accelerationStructureNeedsRebuild(true),
		m_lastEnvMap(nullptr)
	{
	}
	PathTraceStage::~PathTraceStage()
	{

	}

	void PathTraceStage::initialize()
	{
		m_rtStage.initialize(this, getRenderer(), getGraph(), m_matMngr, m_meshMngr);
		RenderGraphNode* rtNode;
		size_t rtSlot;
		m_rtStage.getOutput(&rtNode, rtSlot);

		m_mergeStage.initialize(getRenderer(), getGraph());
		m_mergeStage.setInput(rtNode, rtSlot);
		
		m_accStructureHelper.init(getRenderer());

		m_firstTimeUpdate = true;
	} 
	void PathTraceStage::shutdown()
	{
		m_rtStage.shutdown();
		m_mergeStage.shutdown();
		m_accStructureHelper.deinit();
	}
	 
	void PathTraceStage::onRenderResolutionChanged(const RenderResolutionDependantResourcesData& data)
	{

	
		m_resolveTargetWidth = data.newRenderResolutionWidth;
		m_resolveTargetHeight = data.newRenderResolutionHeight;

		uvec2 newRes = uvec2(m_resolveTargetWidth, m_resolveTargetHeight);

		m_rtStage.onRenderResolutionChanged(data, newRes);
		m_mergeStage.onRenderResolutionChanged(data, newRes);

		clearAccumulatedFrames();
		updateEffectiveRaytraceResolution();
	}

	bool PathTraceStage::hasCameraMoved()
	{
		return getRenderer()->hasViewMoved();
	}
	bool PathTraceStage::hasSceneChanged()
	{
		bool objectsChanged =  getRenderer()->getMaterialManager().hasChanges() || getRenderer()->getMeshManager().hasChanges() || getRenderer()->getRenderObjectManager().hasChanges();
		bool skymapChanged = getRenderer()->getConcreteRendererConfiguration().getRendererVarValueInternal<Texture*>(RVARNAME_SKYBOX) != m_lastEnvMap;
		
		return objectsChanged || skymapChanged;
	}

	void PathTraceStage::onRenderGraphCompiled(const RenderGraphLifetimeData& data)
	{
		m_rtStage.onRenderGraphCompiled(data);
		m_mergeStage.onRenderGraphCompiled(data);
	}



	void PathTraceStage::prepare(const PrepareData& cntx)
	{

	}
	void PathTraceStage::update(const UpdateData& cntx)
	{
		bool sceneHasChanges = hasSceneChanged();

		if (sceneHasChanges || hasCameraMoved())
		{
			clearAccumulatedFrames();
		}

		if (sceneHasChanges || m_firstTimeUpdate)
		{
			updateInstanceOffsets();
			{
				RenderObjectManager& roMngr = getRenderer()->getRenderObjectManager();
				const RenderObjectId* ids = roMngr.getAllIds();
				MaterialPerSubmeshArray* materials = roMngr.getAllMaterials();
				MeshInternal** meshes = roMngr.getAllMeshes();
				size_t entryCount = roMngr.getNumberOfObjects();
				m_rtStage.updateShaderTable(ids, materials, meshes, m_instanceOffsetPerRenderObject.data(), entryCount, m_totalInstanceCount);
			}
			
			m_firstTimeUpdate = false;
			m_accelerationStructureNeedsRebuild = true;

			m_lastEnvMap = getRenderer()->getConcreteRendererConfiguration().getRendererVarValueInternal<Texture*>(RVARNAME_SKYBOX);
		}

		CombineSamplesSubStage::UpdateParams combineUpdate;
		combineUpdate.clearAccumulated = m_framesAccumulated == 0;
		combineUpdate.sourceTextureResolution = uvec2(m_raysPerFrameWidth, m_raysPerFrameHeight);
		combineUpdate.samplesPerPixel = getCurrentNumberOfSamplesPerPixel() + 1;
		combineUpdate.targetOffsetScaleBias = getCurrentResolveTargetTexelOffsetParams();
		m_mergeStage.update(combineUpdate);

		PathIntegratorSubStage::UpdateParams integratorUpdate;
		integratorUpdate.sampleOffset = getCurrentNumberOfSamplesPerPixel();
		integratorUpdate.rayGenOffsetInTexels = getCurrentRayGenerationOffset();
		integratorUpdate.raysPerFrame = uvec2p(m_raysPerFrameWidth, m_raysPerFrameHeight);
		m_rtStage.update(integratorUpdate);


		updateAccumulatedFrames();
	}
	
	void PathTraceStage::updateInstanceOffsets()
	{
		RenderObjectManager& roMngr = getRenderer()->getRenderObjectManager();

		const RenderObjectId* ids = roMngr.getAllIds();
		MaterialPerSubmeshArray* materials = roMngr.getAllMaterials();
		MeshInternal** meshes = roMngr.getAllMeshes();
		size_t entryCount = roMngr.getNumberOfObjects();

		m_instanceOffsetPerRenderObject.resize(entryCount);

		size_t instanceCount = 0;
		for (size_t i = 0; i < entryCount; ++i)
		{
			m_instanceOffsetPerRenderObject[i] = instanceCount;
			instanceCount += meshes[i]->getSubmeshCount();
		}
		m_totalInstanceCount = instanceCount;
	}


	TopLevelAccelerationStructureHandle PathTraceStage::getAccelerationStructure(CommandBufferHandle cmd)
	{
		if (m_accelerationStructureNeedsRebuild)
		{
			m_accStructureHelper.updateBottomLevelStructures(cmd);

			auto assignPerInstanceParams = [this](size_t arrayIndex, RenderObjectId id, size_t submeshIndex, uint32_t& instanceIdOut, uint32_t& instanceMaskOut, size_t& hitGroupShaderTableOffset)
			{
				size_t shdTblOffset = m_instanceOffsetPerRenderObject[id];

				instanceIdOut = (uint32_t)(shdTblOffset + submeshIndex);
				instanceMaskOut = ~0;
				hitGroupShaderTableOffset = (shdTblOffset + submeshIndex);
			};

			m_accStructureHelper.updateTopLevelStructures(cmd, assignPerInstanceParams);
		}
		return m_accStructureHelper.getTopLevelAccelerationStructure();
	}

	RenderStageConnection PathTraceStage::getOutputConnection(size_t id)
	{
		RenderStageConnection conn;
		switch (id)
		{
		case RAYTRACE_STAGE_CONNECTION_COLOR:
		{
			m_mergeStage.getOutput(&conn.node, conn.slot);
			break;
		}
		default:
			assert(false && "Stage connection id not recognized");
		}
		return conn;
	}
	void PathTraceStage::setInputConnection(size_t id, const RenderStageConnection& connection)
	{
		assert(false && "PathTraceStage accepts nothing as input!");
	}

	void PathTraceStage::clearAccumulatedFrames() 
	{ 
		m_framesAccumulated = 0;
	};

	void PathTraceStage::updateAccumulatedFrames()
	{
		++m_framesAccumulated;
	}
	
	void PathTraceStage::updateEffectiveRaytraceResolution()
	{
		m_raysPerFrameWidth = m_resolveTargetWidth / m_raysPerFrameDivisor;
		m_raysPerFrameHeight = m_resolveTargetHeight / m_raysPerFrameDivisor;
	}


	void PathTraceStage::setRayTraceResolutionReductionFactor(uint32_t factor)
	{
		assert(factor < 32);
		m_raysPerFrameDivisor = 1 << factor;

		assert(factor < m_resolveTargetWidth);
		assert(factor < m_resolveTargetHeight);

		clearAccumulatedFrames();
		updateEffectiveRaytraceResolution();
	}

	uvec4p PathTraceStage::getCurrentResolveTargetTexelOffsetParams()
	{    
		//scale & bias
		uint64_t frameIndex = m_framesAccumulated % (m_raysPerFrameDivisor * m_raysPerFrameDivisor);
		return glm::uvec4(m_raysPerFrameDivisor, m_raysPerFrameDivisor, frameIndex % m_raysPerFrameDivisor, frameIndex / m_raysPerFrameDivisor);
	}
	vec2p PathTraceStage::getCurrentRayGenerationOffset()
	{
		glm::uvec4 p = getCurrentResolveTargetTexelOffsetParams();
		glm::uvec2 pixelOffset(p.z, p.w);
		float targetPixelWidth = 1.f / m_resolveTargetWidth;
		float targetPixelHeight = 1.f / m_resolveTargetHeight;

		return vec2p(pixelOffset.x * targetPixelWidth, pixelOffset.y * targetPixelHeight);
	}

	uint64_t PathTraceStage::getCurrentNumberOfSamplesPerPixel()
	{
		uint32_t iterationsToFullResolution = m_raysPerFrameDivisor * m_raysPerFrameDivisor;
		uint64_t v = m_framesAccumulated / iterationsToFullResolution;
		return v;
	}
	
	  
}