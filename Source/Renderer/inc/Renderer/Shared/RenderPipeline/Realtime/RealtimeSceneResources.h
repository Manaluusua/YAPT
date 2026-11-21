#pragma once

#include <Math/Math.h>
#include <Gfx/GfxTypes.h>
#include <Renderer/Shared/SceneInstanceData.h>
#include <Renderer/Shared/Utility/GpuBufferHelper.h>
#include <vector>

namespace YAPT
{
	class CRenderer;
	class RenderResourcesPool;
	class BindlessMaterialManager;
	class BindlessMeshManager;

	//keep in sync with realtimeCommon.hlsl
	struct RealtimeViewConstants
	{
		mat4p worldToClip;
		vec4p renderResolution; //xy: resolution, zw: 1/resolution
	};

	class RealtimeSceneResources
	{
	public:

		struct DrawBucket
		{
			uint32_t meshEntryIndex;
			uint32_t materialEntryIndex;
			uint32_t firstInstance;
			uint32_t instanceCount;
		};

		RealtimeSceneResources();
		~RealtimeSceneResources();

		void init(CRenderer* rend, RenderResourcesPool* pipelineLifetimeResources, BindlessMaterialManager* matMngr, BindlessMeshManager* meshMngr);
		void shutdown();
		void update(uvec2 renderResolution);

		BindlessMaterialManager* getMaterialManager() const { return m_matMngr; }
		BindlessMeshManager* getMeshManager() const { return m_meshMngr; }
		const SceneInstanceData& getInstanceData() const { return m_instanceData; }

		BufferViewHandle getViewConstantsView() { return m_viewConstants.getView(); }

		const std::vector<DrawBucket>& getDrawBuckets() const { return m_drawBuckets; }
		uint32_t getDrawCount() const { return (uint32_t)m_drawBuckets.size(); }
		BufferHandle getDrawArgumentsBuffer() const { return m_drawArgs.getBufferHandle(); }
		BufferViewHandle getDrawInstanceIndicesView() const { return m_drawInstanceIndices.getBufferViewHandle(); }

	private:

		bool hasSceneChanged() const;
		void rebuildDraws();

		CRenderer* m_renderer;
		BindlessMaterialManager* m_matMngr;
		BindlessMeshManager* m_meshMngr;

		SceneInstanceData m_instanceData;
		FixedSizeGpuBufferHelper<RealtimeViewConstants> m_viewConstants;

		DynamicSizeGpuBufferHelper<uint32_t> m_drawInstanceIndices;
		DynamicSizeGpuBufferHelper<DrawIndirectArguments> m_drawArgs;

		std::vector<DrawBucket> m_drawBuckets;
		std::vector<size_t> m_instanceOffsetPerRenderObject;
		bool m_firstUpdate;
	};
}
