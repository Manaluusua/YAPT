#ifndef YAPT_SHARED_COPYTEXTURESTAGE_H
#define YAPT_SHARED_COPYTEXTURESTAGE_H

#include <Renderer/Shared/RenderPipeline/RenderStage.h>
#include <Gfx/RenderGraph/RenderNode.h>
#include <Renderer/Shared/Utility/PostProcessUtility.h>

namespace YAPT
{
	class CopyTextureStage final : public RenderStage
	{
	public:

		enum CopyTextureStageConnection
		{
			COPYTEXTURE_STAGE_CONNECTION_SOURCE,
			COPYTEXTURE_STAGE_CONNECTION_DESTINATION
		};

		CopyTextureStage(ResourceFormat forceDestinationFormat = ResourceFormat::UNKNOWN, ResourceFormat forceSourceformat = ResourceFormat::UNKNOWN);
		~CopyTextureStage();

		virtual void initialize() override;
		virtual void shutdown() override;
		virtual void onRenderResolutionChanged(const RenderResolutionDependantResourcesData& data) override;
		virtual JobHandle prepare(const PrepareData& data) override;
		virtual JobHandle update(const UpdateData& data) override;
		virtual void beforeExecute() override {};

		virtual RenderStageConnection getOutputConnection(size_t id) override;
		virtual void setInputConnection(size_t id, const RenderStageConnection& connection) override;

		TextureHandle getDestinationTexture() const { return m_destinationTexture; }

	private:

		void executeCopy(const RenderGraphNodeExecutionContext& execContext);

		RenderNode* m_copyNode;
		PostProcessGraphicsPassUtility m_copyPass;

		TextureHandle m_destinationTexture;
		ResourceFormat m_destinationFormat;
		ResourceFormat m_sourceFormat;

		uint32_t m_renderWidth;
		uint32_t m_renderHeight;
		bool m_supportReadback;
	};
}
#endif
