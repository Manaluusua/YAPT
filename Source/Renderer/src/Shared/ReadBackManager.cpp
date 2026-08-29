#include <Renderer/Shared/ReadBackManager.h>
#include <Renderer/ReadbackHandle.h>
#include <Renderer/Shared/CRenderer.h>


namespace YAPT
{
	class ReadbackManager;
	

	ReadbackManager::ReadbackManager(CRenderer& renderer)
		:m_renderer(renderer),
		m_freeReadbackObjects(m_renderer.getPipelineLength(), MAX_READBACK_REQUESTS)
	{

	}
	ReadbackManager::~ReadbackManager()
	{

	}

	void ReadbackManager::syncToRenderThread()
	{
		m_freeReadbackObjects.flush();
	}

	ReadbackHandle ReadbackManager::readback(ReadbackTarget target)
	{
		uint32_t index = m_freeReadbackObjects.allocate();
		CReadbackObject* obj = &m_readbackObjectPool[index];
		obj->setIndex(index);
		return RCObjectPtr<ReadbackObject>(obj);
	}

	void ReadbackManager::readbackObjectRelease(CReadbackObject* obj)
	{
		m_freeReadbackObjects.free(obj->getIndex());
		obj->setIndex(0);
	}
}