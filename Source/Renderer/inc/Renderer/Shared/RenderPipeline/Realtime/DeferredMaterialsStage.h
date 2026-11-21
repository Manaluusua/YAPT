#pragma once

#include <Renderer/Shared/RenderPipeline/RenderStage.h>
#include <Gfx/RenderGraph/RenderNode.h>
#include <Renderer/Shared/Utility/PipelineLayoutHelper.h>
#include <Renderer/Shared/Utility/PipelineStateDescriptionUtility.h>

namespace YAPT
{
	class RealtimeSceneResources;

	//rasterizes all instances into a visibility target: x = instance index + 1 (0 = empty), y = primitive id, zw = uv as float bits
	class DeferredMaterialsStage final : public RenderStage
	{
	public:

		enum DeferredMaterialsStageConnection
		{
			DEFERREDMATERIALS_STAGE_CONNECTION_VISIBILITY,
			DEFERREDMATERIALS_STAGE_CONNECTION_DEPTH
		};

		static constexpr ResourceFormat VisibilityFormat = ResourceFormat::RGBA32_UINT;

		DeferredMaterialsStage(RealtimeSceneResources* sceneResources);
		~DeferredMaterialsStage();

		virtual void initialize(const InitializeData& data) override;
		virtual void createNodes() override;
		virtual void shutdown() override;
		virtual void onRenderGraphCompiled(const RenderGraphLifetimeData& data) override;
		virtual void onRenderResolutionChanged(const RenderResolutionDependantResourcesData& data) override;
		virtual JobHandle prepare(const PrepareData& data) override;
		virtual JobHandle update(const UpdateData& data) override;
		virtual void beforeExecute(const ExecuteData& data) override {};

		virtual RenderStageConnection getOutputConnection(size_t id) override;
		virtual void setInputConnection(size_t id, const RenderStageConnection& connection) override;

	private:

		void execute(const RenderGraphNodeExecutionContext& execContext);
		void recreatePso();
		void destroyPso();

		RealtimeSceneResources* m_sceneResources;
		RenderNode* m_renderNode;

		TextureHandle m_visibilityTarget;
		TextureHandle m_depthTarget;

		PipelineLayoutHelper m_layout;
		GraphicsPipelineStateDescHelper m_psoDescHelper;
		GraphicsPipelineStateHandle m_pso;
		DescriptorSetHandle m_descSet;

		VertexInputAttribute m_instanceAttribute;
		VertexBufferDefinition m_instanceBufferDefinition;

		uint64_t m_psoRenderPassSignature;
	};
}
