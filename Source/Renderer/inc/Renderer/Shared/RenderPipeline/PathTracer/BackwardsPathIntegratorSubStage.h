#pragma once

#include <Renderer/Shared/RenderPipeline/PathTracer/PathIntegratorSubStage.h>
#include <Renderer/Shared/RenderPipeline/PathTracer/RaytraceCommonResources.h>
#include <Renderer/Shared/Utility/PostProcessUtility.h>
#include <Renderer/Shared/Utility/ShaderTableHelper.h>
#include <Gfx/RenderGraph/RaytraceNode.h>
namespace YAPT
{
	

	class RenderGraph;
	class BindlessMaterialManager;
	class BindlessMeshManager;
	class BackwardsPathIntegratorSubStage : public PathIntegratorSubStage
	{
	public:
		constexpr static uint32_t RAY_MAX_VOLUMES_ENTERED = 4;

		BackwardsPathIntegratorSubStage(bool useNextEventEstimation);
		virtual ~BackwardsPathIntegratorSubStage();

		virtual void initialize(AccelerationStructureProvider* accStructProvider, CRenderer* rend, RenderGraph* graph, BindlessMaterialManager* matMngr, BindlessMeshManager* meshMngr) final;
		virtual void shutdown() final;
		virtual void onRenderGraphCompiled(const RenderStage::RenderGraphLifetimeData& data) final;
		virtual void onRenderResolutionChanged(const RenderStage::RenderResolutionDependantResourcesData& data, uvec2 newResolution) final;
		virtual void prepare(const RenderStage::PrepareData& params) final;
		virtual void update(const UpdateParams& params) final;
		virtual void getOutput(RenderGraphNode** node, size_t& slotOut) final;

		virtual void sceneChanged(const PathIntegratorSubStage::SceneData& sceneData) final;

	private:

		struct RaytracePayload
		{
			float absorption[SPECTRAL_SAMPLES_COUNT * RAY_MAX_VOLUMES_ENTERED];
			float throughput[SPECTRAL_SAMPLES_COUNT];
			float totalLight[SPECTRAL_SAMPLES_COUNT];
			float ior[RAY_MAX_VOLUMES_ENTERED];
			vec3p rayOrigin;
			uint32_t rayIndex;
			vec3p rayDirection;
			uint32_t pathLength;

			uint32_t numberVolumesEntered;
			uint32_t rayState;
			uint32_t spectralSampleSetIndex;
			uint32_t flags;
		};

		struct RayHitShaderTableConstantData
		{
			uvec2p materialAndMeshIndices;
		};


		struct RayMissShaderTableConstantData
		{
			uint32_t envTextureIndex;
			uint32_t envType;
		};	
		//stride and offset are assumed in dwords (uint32/float32) in the shader
		void writeShaderTableEntryAndConstantData(RenderObjectId id, const MaterialPerSubmeshArray& mat, const MeshIndex meshID, size_t submeshIndex, ShaderTableEntry* entry);

		void executeRaytrace(const RenderGraphNodeExecutionContext& exec);

		RenderGraph* m_graph;
		CRenderer* m_renderer;
		RaytraceCommonResources m_raytraceCommon;
		AccelerationStructureProvider* m_accStructProvider;
		BindlessMaterialManager* m_materialMngr;
		BindlessMeshManager* m_meshMngr;

		ShaderTableHelper m_shaderTableHelper;

		RaytraceNode* m_rtNode;
		PipelineLayoutHelper m_rtLayout;
		RaytracePipelineStateHandle m_raytracePso;
		DescriptorSetHandle m_rtCommonResourcesDescSet;
		DescriptorSetHandle m_rtMiscResourcesDescSet;

		uvec2 m_renderResolution;
		UpdateParams m_lastUpdateParams;
		bool m_useNEE;
	};
}
