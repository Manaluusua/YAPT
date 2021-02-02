#include <Renderer/Shared/RenderView.h>
#include <Common/ThreadPool.h>
#include <Renderer/Shared/RenderObjectManager.h>
#include <Renderer/Shared/Utility/RenderAPIAbstractionUtility.h>
#include <Common/CommonUtilities.h>

namespace YAPT
{

	RenderView::RenderView(GfxApiHandle gfx)
		:m_gfx(gfx),
		m_gpuData(gfx)
	{

	}

	void RenderView::setView(const mat4& view)
	{
		m_viewEngine = view;
		refreshInternal();
	}
	const mat4& RenderView::getView() const
	{
		return m_viewEngine;
	}

	void RenderView::setProjection(const mat4& proj)
	{
		m_projectionEngine = proj;
		refreshInternal();
	}
	const mat4& RenderView::getProjection() const
	{
		return m_projectionEngine;
	}

	const mat4& RenderView::getViewProjectionEngine() const
	{
		return m_viewProjEngine;
	}
	const mat4& RenderView::getViewProjectionPlatform() const
	{
		return m_viewProjPlatform;
	}

	const mat4& RenderView::getProjectionPlatform() const
	{
		return m_projectionPlatform;
	}

	void RenderView::setNearFar(const vec2& nearFar)
	{
		m_nearFar = nearFar;
	}
	const vec2& RenderView::getNearFar() const
	{
		return m_nearFar;
	}

	void RenderView::refreshInternal()
	{
		m_viewProjEngine = m_projectionEngine * m_viewEngine;

		m_projectionPlatform = fromCommonNDCtoPlatformSpecificNDC() * m_projectionEngine;
		m_viewProjPlatform = fromCommonNDCtoPlatformSpecificNDC() * m_viewProjEngine;

	}

	void RenderView::issueViewDependantRenderObjectJobs(ThreadPool& pool, RenderObjectManager& renderObjectManager)
	{
		constexpr size_t MAX_JOBS = 4;
		YAPT::mat4* wMat = renderObjectManager.getAllMatrices();
		size_t count = renderObjectManager.getNumberOfObjects();
		size_t numberOfJobs = max(size_t(1), min(size_t(MAX_JOBS), count / 10u));
		size_t operationsPerJob = (count + numberOfJobs - 1) / numberOfJobs;

		if (count == 0)
		{
			return;
		}

		m_calculateMVPWorkItems.resize(numberOfJobs);

		m_renderObjectMVPs.resize(count);

		auto calculateMvpFunc = [](void* usrData)
		{
			CalculateMVPWorkItem* item = static_cast<CalculateMVPWorkItem*>(usrData);
			for (size_t i = 0; i < item->count; ++i)
			{
				item->mvpArray[i] = item->viewProj * item->worldMatrixArray[i];
			}



		};

		size_t offset = 0;
		for (size_t i = 0; i < numberOfJobs; ++i)
		{
			CalculateMVPWorkItem& item = m_calculateMVPWorkItems[i];
			item.viewProj = m_viewProjPlatform;
			item.mvpArray = m_renderObjectMVPs.data() + offset;
			item.worldMatrixArray = wMat + offset;
			item.count = min(operationsPerJob, count - offset);
			offset += item.count;

		}

		
		for (size_t i = 0; i < numberOfJobs; ++i)
		{
			pool.addTask(calculateMvpFunc, &m_calculateMVPWorkItems[i]);
		}
		
	}


	void RenderView::updatePerPerViewObjectGPUData()
	{
		const size_t GROW_STEP = 128;

		if (m_gpuData.getAllocatedEntryCount() < m_renderObjectMVPs.size())
		{
			size_t newEntries = align(m_renderObjectMVPs.size(), GROW_STEP);
			m_gpuData.allocate(newEntries);
		}
		char* data = m_gpuData.map(0, m_renderObjectMVPs.size());
		for (size_t i = 0; i < m_renderObjectMVPs.size(); ++i)
		{
			memcpy(data + m_gpuData.getAlignedEntrySize() * i, &m_renderObjectMVPs[i], sizeof(PerObjectPerViewGPUData));
		}
		m_gpuData.unmap();
	}
}