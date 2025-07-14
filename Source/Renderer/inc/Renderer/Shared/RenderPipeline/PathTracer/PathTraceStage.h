#pragma once

#include <Renderer/Shared/RenderPipeline/PathTracer/RaytraceCommonResources.h>

#include <Renderer/Shared/RenderPipeline/RenderStage.h>
#include <Renderer/Shared/RenderPipeline/PathTracer/CombineSamplesSubStage.h>
#include <Gfx/RenderGraph/RaytraceNode.h>
#include <Gfx/RenderGraph/RenderNode.h>
#include <Gfx/RenderGraph/GenericExecuteNode.h>
#include <Renderer/Shared/Utility/PostProcessUtility.h>
#include <Renderer/Shared/Utility/AccelerationStructureHelper.h>
#include <Renderer/Shared/Utility/ShaderTableHelper.h>
#include <spectralConstants.h>


namespace YAPT
{
	class Texture;
	class BindlessMaterialManager;
	class BindlessMeshManager;
	class PathIntegratorSubStage;
	class PathTraceStage final : public RenderStage, PathIntegratorSubStage::AccelerationStructureProvider
	{
	public:
		enum class PathIntegratorType
		{
			BACKWARDS,
			BACKWARDS_WITH_NEE,
			BIDIRECTIONAL
		};

		enum RaytraceStageConnection
		{
			RAYTRACE_STAGE_CONNECTION_COLOR
		};
		 
		PathTraceStage(BindlessMaterialManager* matMngr, BindlessMeshManager* meshMngr, PathIntegratorType type);
		virtual ~PathTraceStage();

		//RenderStage
		virtual void initialize() final;
		virtual void shutdown() final;
		virtual void onRenderGraphCompiled(const RenderGraphLifetimeData& data) final;
		virtual void onRenderResolutionChanged(const RenderResolutionDependantResourcesData& data) final;
		virtual void prepare(const PrepareData& data) final;
		virtual void update(const UpdateData& data) final;

		virtual RenderStageConnection getOutputConnection(size_t id) final;
		virtual void setInputConnection(size_t id, const RenderStageConnection& connection) final;

		//AccelerationStructureProvider
		virtual void prepareAccelerationStructure() final;
		virtual TopLevelAccelerationStructureHandle getAccelerationStructure() final;

		void setRayTraceResolutionReductionFactor(uint32_t factor); //0 fullres, 1 is dimensions/2^1, 2 is dimensions/2^2 etc 

	private:
		
		bool hasCameraMoved();
		bool hasSceneChanged();
		void clearAccumulatedFrames();
		void updateEffectiveRaytraceResolution();
		void updateAccumulatedFrames();
		uvec4p getCurrentResolveTargetTexelOffsetParams();
		vec2p getCurrentRayGenerationOffset();
		uint64_t getCurrentNumberOfSamplesPerPixel();
		void updateInstanceOffsets();

		CommandBufferPoolHandle m_cmdBufferPool;

		BindlessMaterialManager* m_matMngr;
		BindlessMeshManager* m_meshMngr;

		AccelerationStructureHelper m_accStructureHelper;
		std::vector<size_t> m_instanceOffsetPerRenderObject;
		size_t m_totalInstanceCount;

		PathIntegratorSubStage* m_rtStage;
		CombineSamplesSubStage m_mergeStage;

		uint32_t m_raysPerFrameWidth;
		uint32_t m_raysPerFrameHeight;

		uint32_t m_resolveTargetWidth;
		uint32_t m_resolveTargetHeight;

		uint64_t m_framesAccumulated;
		uint32_t m_raysPerFrameDivisor;


		Texture* m_lastEnvMap;

		bool m_accelerationStructureNeedsRebuild;
		bool m_firstTimeUpdate;
	};
}
