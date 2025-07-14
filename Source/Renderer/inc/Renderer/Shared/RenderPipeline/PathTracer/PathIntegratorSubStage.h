#pragma once

#include <Renderer/Shared/CRenderer.h>
#include <Renderer/Shared/RenderPipeline/RenderStage.h>
#include <Gfx/RenderGraph/RenderNode.h>
#include <spectralConstants.h>
#include <Gfx/GfxTypes.h>

namespace YAPT
{
	class BindlessMaterialManager;
	class BindlessMeshManager;

	class PathIntegratorSubStage
	{
	public:
		struct UpdateParams
		{
			const RenderStage::UpdateData* stageUpdateContext;
			size_t sampleOffset;
			uvec2p rayGenOffsetInTexels;
			uvec2p raysPerFrame;
		};

		struct SceneData
		{
			const RenderObjectId* ids;
			const MaterialPerSubmeshArray* materials;
			const MeshIndex* meshes;
			const size_t* instanceOffsetPerRenderObject;
			const mat4* objToWorldMatrices;
			const mat4* worldToObjMatrices;
			size_t objectCount;
			size_t instancesCount;
		};

		class AccelerationStructureProvider
		{
		public:
			virtual void prepareAccelerationStructure() = 0;
			virtual TopLevelAccelerationStructureHandle getAccelerationStructure() = 0;

			virtual ~AccelerationStructureProvider() {}
		};

		virtual void initialize(AccelerationStructureProvider* accStructProvider, CRenderer* rend, RenderGraph* graph, BindlessMaterialManager* matMngr, BindlessMeshManager* meshMngr) = 0;
		virtual void shutdown() = 0;
		virtual void onRenderGraphCompiled(const RenderStage::RenderGraphLifetimeData& data) = 0;
		virtual void onRenderResolutionChanged(const RenderStage::RenderResolutionDependantResourcesData& data, uvec2 newResolution) = 0;
		virtual void prepare(const RenderStage::PrepareData& params) = 0;
		virtual void update(const UpdateParams& params) = 0;
		

		virtual void getOutput(RenderGraphNode** node, size_t& slotOut) = 0;
		virtual void sceneChanged(const SceneData& sceneData) = 0;

		virtual ~PathIntegratorSubStage() {}
	};
}