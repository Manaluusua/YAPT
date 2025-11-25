#ifndef YAPT_SHARED_MESHDEBUGSTAGE_H
#define YAPT_SHARED_MESHDEBUGSTAGE_H

#include <Renderer/Shared/RenderPipeline/RenderStage.h>
#include <Gfx/RenderGraph/RenderNode.h>
#include <Renderer/Shared/Utility/PostProcessUtility.h>
#include <Renderer/Shared/MeshInternal.h>
#include <Renderer/Shared/RenderObjectManager.h>
#include <Renderer/Shared/Utility/RenderUtility.h>
namespace YAPT
{
	class MeshDebugStage final : public RenderStage
	{
	public:

		enum MeshDebugStageConnection
		{
			MESHDEBUG_STAGE_CONNECTION_COLOR,
			MESHDEBUG_STAGE_CONNECTION_DEPTH
		};
		MeshDebugStage(bool outputDirectlyToSwapchain);
		~MeshDebugStage();

		virtual void initialize() override;
		virtual void shutdown() override;
		virtual void onRenderGraphCompiled(const RenderGraphLifetimeData& data) override;
		virtual void onRenderResolutionChanged(const RenderResolutionDependantResourcesData& data) override;
		virtual void prepare(const PrepareData& data) override;
		virtual void update(const UpdateData& data) override;
		virtual void beforeExecute() override {};

		virtual RenderStageConnection getOutputConnection(size_t id) override;
		virtual void setInputConnection(size_t id, const RenderStageConnection& connection) override;

	private:

		struct PSOEntry
		{
			GraphicsPipelineStateHandle pso;
			MeshToVertexInputLayoutMappingUtility vertexBufferMapping;
		};

		void replicateRenderVars();

		void updateRenderObjectChanges();
		void sortRenderObjectsToBuckets();

		void clearPSOs();

		void execute(const RenderGraphNodeExecutionContext& execContext);

		void assignToPSOBucket(MeshInternal* mesh, RenderObjectManager::RenderData& data);

		RenderNode* m_renderMeshesNode;

		TextureHandle m_colorTarget;
		TextureHandle m_depthTarget;

		uint32_t m_renderWidth;
		uint32_t m_renderHeight;

		PipelineLayoutHelper m_layout;
		GraphicsPipelineStateDescHelper m_psoDescHelper;
		DescriptorSetHandle m_descSet;
		
		std::unordered_map< MeshLayoutID, size_t> m_meshLayoutToPSOIndex;
		std::vector<PSOEntry> m_pipelineStateObjects;
		std::vector<AttributeSemantic> m_vertexInputSemantics;
		std::vector<std::vector<size_t>> m_renderObjectBuckets;
		size_t m_renderDataIndex;
		bool m_refreshAllRenderObjects;
		bool m_outputToSwapchain;

		
	};
}
#endif