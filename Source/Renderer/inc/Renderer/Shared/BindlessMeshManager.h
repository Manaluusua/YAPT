#pragma once

#include <Math/Math.h>
#include <Renderer/Shared/Utility/GpuBufferHelper.h>
#include <Renderer/Shared/MeshManager.h>
namespace YAPT
{

	struct MeshEntryGPU
	{
		uvec2p positionBuffer;
		uvec2p indexBuffer;
		uvec2p normalBuffer;
		uvec2p tangentBuffer;
		uvec2p uvBuffer;
		uint32_t indexCount;
		uint32_t pad0;
	};

	class CRenderer;
	class BindlessMeshManager
	{
	public:
		BindlessMeshManager();
		~BindlessMeshManager();

		void init(CRenderer* renderer);
		void update();
		void shutdown();

		BufferViewHandle getBufferViewHandle() const { return m_gpuBuffer.getBufferViewHandle(); }

		size_t getEntryIndexForMeshIdAndSubmesh(MeshIndex id, size_t submeshIndex) { return m_meshIDToBufferIndex[id] + submeshIndex; }

	private:

		void syncMeshStateToGPU();
		CRenderer* m_renderer;
		DynamicSizeGpuBufferHelper<MeshEntryGPU> m_gpuBuffer;
		std::vector<size_t> m_meshIDToBufferIndex;
	};
}