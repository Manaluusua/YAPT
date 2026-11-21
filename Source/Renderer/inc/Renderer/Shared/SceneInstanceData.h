#pragma once

#include <Math/Math.h>
#include <Renderer/Shared/Utility/GpuBufferHelper.h>
#include <Renderer/Shared/RenderObjectManager.h>
#include <vector>

namespace YAPT
{
	class CRenderer;
	class BindlessMaterialManager;
	class BindlessMeshManager;

	struct RenderObjectTransformDataGPU
	{
		vec4p objToWorldR0;
		vec4p objToWorldR1;
		vec4p objToWorldR2;
		vec4p worldToObjectR0;
		vec4p worldToObjectR1;
		vec4p worldToObjectR2;
	};

	//per instance (render object * submesh) transforms and material/mesh entry indices, keep in sync with sceneInstanceData.hlsl
	class SceneInstanceData
	{
	public:

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

		SceneInstanceData();
		~SceneInstanceData();

		void init(CRenderer* rend, BindlessMaterialManager* matMngr, BindlessMeshManager* meshMngr);
		void shutdown();
		void update(const SceneData& sceneData);

		BufferViewHandle getTransformsBufferView() const { return m_transformData.getBufferViewHandle(); }
		BufferViewHandle getMaterialAndMeshIndicesBufferView() const { return m_materialAndMeshIndices.getBufferViewHandle(); }
		size_t getInstanceOffset(size_t renderObjectDataIndex) const { return m_instanceOffsetPerRenderObject[renderObjectDataIndex]; }

		static size_t computeInstanceOffsets(CRenderer* rend, std::vector<size_t>& offsetsOut);
		static SceneData gatherSceneData(CRenderer* rend, const std::vector<size_t>& instanceOffsets, size_t instanceCount);

	private:
		CRenderer* m_renderer;
		BindlessMaterialManager* m_materialMngr;
		BindlessMeshManager* m_meshMngr;
		DynamicSizeGpuBufferHelper<uvec2p> m_materialAndMeshIndices;
		DynamicSizeGpuBufferHelper<RenderObjectTransformDataGPU> m_transformData;
		std::vector<size_t> m_instanceOffsetPerRenderObject;
	};
}
