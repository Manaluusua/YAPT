#pragma once

#include <Renderer/Shared/CRenderer.h>
#include <Renderer/Shared/SceneInstanceData.h>
#include <Renderer/Shared/RenderPipeline/RenderStage.h>
#include <Gfx/RenderGraph/RenderNode.h>
#include <spectralConstants.h>
#include <Gfx/GfxTypes.h>
#include <Common/JobSystem.h>

namespace YAPT
{
	class BindlessMaterialManager;
	class BindlessMeshManager;

	

	class PathIntegratorSubStage
	{
	public:

		enum class OutputResource
		{
			COLOR,
			MATERIAL_PARAMS0,
			MATERIAL_PARAMS1
		};

		static constexpr ResourceFormat SampleImageFormat = ResourceFormat::RGBA32_SFLOAT;

		struct UpdateParams
		{
			const RenderStage::UpdateData* stageUpdateContext;
			size_t sampleOffset;
			uvec4p targetOffsetScaleBias;
			uvec2p raysPerFrame;
		};

		using SceneData = SceneInstanceData::SceneData;

		class AccelerationStructureProvider
		{
		public:
			virtual void prepareAccelerationStructure() = 0;
			virtual TopLevelAccelerationStructureHandle getAccelerationStructure() = 0;

			virtual ~AccelerationStructureProvider() {}
		};

		virtual void initialize(AccelerationStructureProvider* accStructProvider, CRenderer* rend, RenderGraph* graph, BindlessMaterialManager* matMngr, BindlessMeshManager* meshMngr, const RenderStage::InitializeData& data) = 0;
		virtual void createNodes() = 0;
		virtual void shutdown() = 0;
		virtual void onRenderGraphCompiled(const RenderStage::RenderGraphLifetimeData& data) = 0;
		virtual void onRenderResolutionChanged(const RenderStage::RenderResolutionDependantResourcesData& data, uvec2 newResolution) = 0;
		virtual JobHandle prepare(const RenderStage::PrepareData& params) = 0;
		virtual JobHandle update(const UpdateParams& params) = 0;
		

		virtual void getOutput(OutputResource resource, RenderGraphNode*& node, size_t& slotOut) = 0;
		virtual void sceneChanged(const SceneData& sceneData) = 0;

		virtual ~PathIntegratorSubStage() {}
	};
}