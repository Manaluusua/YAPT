#pragma once

#include <Renderer/Shared/RenderPipeline/PathTracer/PathIntegratorSubStage.h>
#include <Renderer/Shared/RenderPipeline/PathTracer/RaytraceCommonResources.h>
#include <Renderer/Shared/Utility/PostProcessUtility.h>
#include <Renderer/Shared/Utility/ShaderTableHelper.h>
#include <Gfx/RenderGraph/ComputeNode.h>

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
		virtual void getOutput(OutputResource resource, RenderGraphNode*& node, size_t& slotOut) final;

		virtual void sceneChanged(const PathIntegratorSubStage::SceneData& sceneData) final;

	private:
		void recreateTexelCoordinatePermutationTexture(const RenderStage::RenderResolutionDependantResourcesData& data);

		struct BidirectionalPathTraceConstants
		{
			uvec4 lightAndCameraPathConstraints;
			uvec2 lightPathsPerDim;
			uvec2 cameraRayWorkGroupCount;
			uint32_t cameraWorkGroupSwizzleOffset;
			uint32_t maxVerticesPerLightPath;
			uint32_t maxAllocatedVertices;
			uint32_t maxCameraPathVertices;
		};

		struct LightPathNodePacked
		{
			uvec4p data0;
			uvec4p data1;
			uvec4p data2;
			uvec4p data3;
			uvec4p data4;
		};

		
		
		void executeResetCounters(const RenderGraphNodeExecutionContext& exec);
		void executeCameraPathPass(const RenderGraphNodeExecutionContext& exec);
		void executeLightPathSortPass(const RenderGraphNodeExecutionContext& exec);
		void executeLightPathPass(const RenderGraphNodeExecutionContext& exec);

		void executePreparePureLightPathResources(const RenderGraphNodeExecutionContext& exec);
		void executeCalculatePureLightPaths(const RenderGraphNodeExecutionContext& exec);
		void executeAccumulatePureLightPaths(const RenderGraphNodeExecutionContext& exec);

		RenderGraph* m_graph;
		CRenderer* m_renderer;
		RaytraceCommonResources m_raytraceCommon;
		AccelerationStructureProvider* m_accStructProvider;
		BindlessMaterialManager* m_materialMngr;
		BindlessMeshManager* m_meshMngr;

		ComputeNode* m_resetCountersNode;
		PostProcessComputePassUtility m_resetCountersNodeUtility;

		ComputeNode* m_lightPathsNode;
		PostProcessComputePassUtility m_lightPathHelperUtility;

		ComputeNode* m_lightPathsSortNode;
		PostProcessComputePassUtility m_lightPathSortHelperUtility;

		ComputeNode* m_cameraPathsNode;
		PostProcessComputePassUtility m_cameraPathHelperUtility;

		ComputeNode* m_initPureLightPathResourcesNode;
		PostProcessComputePassUtility m_initPureLightPathsUtility;

		ComputeNode* m_calculatePureLightPathResourcesNode;
		PostProcessComputePassUtility m_calculatePureLightPathsUtility;

		ComputeNode* m_accumulatePureLightPathResourcesNode;
		PostProcessComputePassUtility m_accumulatePathHelperUtility;

		FixedSizeGpuBufferHelper<BidirectionalPathTraceConstants> m_constantsGPU;
		
		DynamicSizeGpuBufferHelper<uint32_t> m_lightPathHeadersGPU;
		DynamicSizeGpuBufferHelper<LightPathNodePacked> m_lightPathsGPU[2];
		FixedSizeGpuBufferHelper<uvec4> m_countersGPU;

		DynamicSizeGpuBufferHelper<uvec2> m_pureLightPathHeadersGPU;
		DynamicSizeGpuBufferHelper<uvec4> m_pureLightPathNodesGPU;

		TextureHandle m_texelRemapTexture;
		TextureViewHandle m_texelRemapTextureView;

		uvec2 m_renderResolution;
		UpdateParams m_lastUpdateParams;

		uint32_t m_maxVerticesPerLightPath;
		uint32_t m_maxVerticesPerCameraPath;
		uvec2 m_pixelsPerLightPath;
	};
}
