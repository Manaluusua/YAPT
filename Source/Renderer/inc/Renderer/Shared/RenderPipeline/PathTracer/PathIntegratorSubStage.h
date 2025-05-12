#pragma once

#include <Renderer/Shared/RenderPipeline/PathTracer/RaytraceCommonResources.h>
#include <Renderer/Shared/CRenderer.h>
#include <Renderer/Shared/RenderPipeline/RenderStage.h>
#include <Gfx/RenderGraph/RenderNode.h>
#include <spectralConstants.h>
#include <Gfx/GfxTypes.h>

namespace YAPT
{
	class PathIntegratorSubStage
	{
	public:
		struct UpdateParams
		{
			size_t sampleOffset;
			uvec2p rayGenOffsetInTexels;
			uvec2p raysPerFrame;
		};

		virtual void initialize(AccelerationStructureProvider* accStructProvider, CRenderer* rend, RenderGraph* graph, BindlessMaterialManager* matMngr, BindlessMeshManager* meshMngr) = 0;
		virtual void shutdown() = 0;
		virtual void onRenderGraphCompiled(const RenderStage::RenderGraphLifetimeData& data) = 0;
		virtual void onRenderResolutionChanged(const RenderStage::RenderResolutionDependantResourcesData& data, uvec2 newResolution) = 0;
		virtual void prepare(const RenderStage::PrepareData& params) = 0;
		virtual void update(const UpdateParams& params) = 0;
		

		virtual void getOutput(RenderGraphNode** node, size_t& slotOut) = 0;
		virtual void sceneChanged(const RenderObjectId* ids, MaterialPerSubmeshArray* materials, MeshInternal** meshes, size_t* instanceOffsetPerRenderObject, size_t objectCount, size_t instancesCount) = 0;

		virtual ~PathIntegratorSubStage() {}
	};
}