#pragma once

#include <Renderer/Shared/GfxApi.h>
#include <Renderer/Shared/Utility/GpuBufferHelper.h>
#include <Math/Math.h>

#include <vector>

namespace YAPT
{
	class RenderObjectManager;
	class ThreadPool;
	class RenderView
	{
	public:

		RenderView(GfxApiHandle gfx);

		void setView(const mat4& view);
		const mat4& getView() const;

		void setProjection(const mat4& proj);
		const mat4& getProjection() const;

		const mat4& getViewProjectionEngine() const;
		const mat4& getViewProjectionPlatform() const;
		const mat4& getProjectionPlatform() const;

		void setNearFar(const vec2& nearFar);
		const vec2& getNearFar() const;

		void issueViewDependantRenderObjectJobs(ThreadPool& pool, RenderObjectManager& renderObjectManager);

		void updatePerPerViewObjectGPUData();

		BufferViewHandle getMVPBufferView() const { return m_gpuData.getBufferViewHandle(); }
		size_t getMVPBufferPerEntrySize() const { return m_gpuData.getAlignedEntrySize(); }

	private:

		struct CalculateMVPWorkItem
		{
			mat4 viewProj;
			mat4* mvpArray;
			mat4* worldMatrixArray;
			size_t count;
			
		};

		struct PerObjectPerViewGPUData
		{
			mat4 mvpMatrix;
		};

		void refreshInternal();

		GfxApiHandle m_gfx;

		mat4 m_viewEngine;
		mat4 m_projectionEngine;
		mat4 m_viewProjEngine;
		mat4 m_projectionPlatform;
		mat4 m_viewProjPlatform;
		vec2 m_nearFar;
		DynamicSizeGpuBufferHelper<PerObjectPerViewGPUData> m_gpuData;
		std::vector<mat4> m_renderObjectMVPs;
		std::vector<CalculateMVPWorkItem> m_calculateMVPWorkItems;
	};
}