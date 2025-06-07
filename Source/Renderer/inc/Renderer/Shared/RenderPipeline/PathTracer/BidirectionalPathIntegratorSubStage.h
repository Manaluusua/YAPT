#pragma once

#include <Renderer/Shared/RenderPipeline/PathTracer/PathIntegratorSubStage.h>
#include <Renderer/Shared/RenderPipeline/PathTracer/RaytraceCommonResources.h>
#include <Renderer/Shared/Utility/PostProcessUtility.h>
#include <Renderer/Shared/Utility/ShaderTableHelper.h>
#include <Gfx/RenderGraph/ComputeNode.h>
#include <array>

namespace YAPT
{
	

	class RenderGraph;
	class BindlessMaterialManager;
	class BindlessMeshManager;
	class BidirectionalPathIntegratorSubStage : public PathIntegratorSubStage
	{
	public:

		constexpr static uint32_t RAY_MAX_VOLUMES_ENTERED = 4;

		BidirectionalPathIntegratorSubStage();
		virtual ~BidirectionalPathIntegratorSubStage();

		virtual void initialize(AccelerationStructureProvider* accStructProvider, CRenderer* rend, RenderGraph* graph, BindlessMaterialManager* matMngr, BindlessMeshManager* meshMngr) final;
		virtual void shutdown() final;
		virtual void onRenderGraphCompiled(const RenderStage::RenderGraphLifetimeData& data) final;
		virtual void onRenderResolutionChanged(const RenderStage::RenderResolutionDependantResourcesData& data, uvec2 newResolution) final;
		virtual void prepare(const RenderStage::PrepareData& params) final;
		virtual void update(const UpdateParams& params) final;
		virtual void getOutput(RenderGraphNode** node, size_t& slotOut) final;

		virtual void sceneChanged(const RenderObjectId* ids, MaterialPerSubmeshArray* materials, MeshInternal** meshes, size_t* instanceOffsets, size_t objectCount, size_t instancesCount) final;

		void setWorldBounds(AABB bounds)
		{
			m_worldBounds = bounds;
		}
	private:

		struct BidirectionalPathTraceConstants
		{
			uint32_t envTextureIndex;
			uint32_t envType;
		};

		struct RenderObjectEntry
		{
			uvec2p materialAndMeshIndices;
		};

		struct LightEntryGPU
		{
			mat4p transform;
			uint32_t meshIndex;
			uint32_t matIndex;
		};

		struct CombineBoundsJobItem
		{
			const AABB* objectBounds;
			size_t boundsOffset;
			size_t boundsCount;
			AABB combinedBounds;
		};

		void setupWorldBoundsJob(ThreadPool* threadPool);
		void setupLightDataJob(ThreadPool* threadPool);

		void executeCameraPathPass(const RenderGraphNodeExecutionContext& exec);


		RenderGraph* m_graph;
		CRenderer* m_renderer;
		RaytraceCommonResources m_raytraceCommon;
		AccelerationStructureProvider* m_accStructProvider;
		BindlessMaterialManager* m_materialMngr;
		BindlessMeshManager* m_meshMngr;

		ComputeNode* m_cameraPathsNode;
		PostProcessComputePassUtility m_cameraPathHelperUtility;

		FixedSizeGpuBufferHelper<BidirectionalPathTraceConstants> m_constantsGPU;
		DynamicSizeGpuBufferHelper<RenderObjectEntry> m_renderObjectsGPU;

		DynamicSizeGpuBufferHelper<LightEntryGPU> m_lightDataGPU;
		AABB m_worldBounds;

		std::array<CombineBoundsJobItem, 8> m_combineBoundsJobs;

		uvec2 m_renderResolution;
		UpdateParams m_lastUpdateParams;
	};
}
