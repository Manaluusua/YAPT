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

		virtual void sceneChanged(const RenderObjectId* ids, MaterialPerSubmeshArray* materials, MeshIndex* meshes, size_t* instanceOffsets, size_t objectCount, size_t instancesCount) final;

	private:

		struct BidirectionalPathTraceConstants
		{
			vec4p worldBoundsMin;
			vec4p worldBoundsMax;
			uvec2 lightPathsPerDim;
			uint32_t envTextureIndex;
			uint32_t envType;
			uint32_t maxVerticesPerLightPath;
			uint32_t maxAllocatedVertices;
		};

		

		struct CombineBoundsJobItem
		{
			const AABB* objectBounds;
			size_t boundsOffset;
			size_t boundsCount;
			AABB combinedBounds;
		};

		struct LightPathHeader
		{
			uvec2 offsetAndCount;
		};

		struct LightPathNode
		{
			vec4p throughput;
			vec4p normalWSPDF;
			vec4p positionWSMISSum;
			vec4p instancePrimitiveBarycentrics;
		};

		void setupWorldBoundsJob(ThreadPool* threadPool);
		

		void executeCameraPathPass(const RenderGraphNodeExecutionContext& exec);
		void executeLightPathSortPass(const RenderGraphNodeExecutionContext& exec);
		void executeLightPathPass(const RenderGraphNodeExecutionContext& exec);
		

		RenderGraph* m_graph;
		CRenderer* m_renderer;
		RaytraceCommonResources m_raytraceCommon;
		AccelerationStructureProvider* m_accStructProvider;
		BindlessMaterialManager* m_materialMngr;
		BindlessMeshManager* m_meshMngr;

		ComputeNode* m_lightPathsNode;
		PostProcessComputePassUtility m_lightPathHelperUtility;

		ComputeNode* m_lightPathsSortNode;
		PostProcessComputePassUtility m_lightPathSortHelperUtility;

		ComputeNode* m_cameraPathsNode;
		PostProcessComputePassUtility m_cameraPathHelperUtility;

		FixedSizeGpuBufferHelper<BidirectionalPathTraceConstants> m_constantsGPU;
		

		DynamicSizeGpuBufferHelper<LightPathHeader> m_lightPathHeadersGPU;
		DynamicSizeGpuBufferHelper<LightPathNode> m_lightPathsGPU[2];
		FixedSizeGpuBufferHelper<uvec3> m_countersGPU;

		std::array<CombineBoundsJobItem, 8> m_combineBoundsJobs;

		uvec2 m_renderResolution;
		UpdateParams m_lastUpdateParams;

		uint32_t m_maxVerticesPerLightPath;
		uvec2 m_pixelsPerLightPath;
	};
}
