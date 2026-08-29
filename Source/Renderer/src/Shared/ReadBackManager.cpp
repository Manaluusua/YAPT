#include <Renderer/Shared/ReadBackManager.h>
#include <Renderer/ReadbackHandle.h>
#include <Renderer/Shared/CRenderer.h>


namespace YAPT
{
	class ReadbackManager;
	

	ReadbackManager::ReadbackManager(CRenderer& renderer)
		:m_renderer(renderer),
		m_freeReadbackObjects(m_renderer.getPipelineLength(), MAX_READBACK_REQUESTS),
		m_activeReadbackObjectCount(0)
	{

	}
	ReadbackManager::~ReadbackManager()
	{

	}

	void ReadbackManager::commitChanges()
	{
		int objectsCount = m_activeReadbackObjectCount.load();
		for (int i = 0; i < objectsCount; ++i)
		{
			uint32_t index = m_activeReadbackObjects[i];
			CReadbackObject& obj =  m_readbackObjectPool[index];
			switch (obj.getStateNonAtomic())
			{
			case ReadbackState::Freed:
			{
				freeResource(index, obj.getReadbackTarget());
				m_freeReadbackObjects.free(obj.getIndex());
				obj.setIndex(0);
				std::swap(m_activeReadbackObjects[i], m_activeReadbackObjects[objectsCount - 1]);
				--objectsCount;
				--i;
			}
			break;

			case ReadbackState::Created:
			{
				allocateResource(obj.getIndex(), obj.getReadbackTarget());
			}
			break;

			}
					
		}
		m_activeReadbackObjectCount.store(objectsCount);
		m_freeReadbackObjects.flush();
	}

	ReadbackHandle ReadbackManager::readback(ReadbackTarget target)
	{
		uint32_t index = m_freeReadbackObjects.allocate();
		CReadbackObject& obj = m_readbackObjectPool[index];
		obj.setIndex(index);
		uint32_t activeArrayIndex = m_activeReadbackObjectCount.fetch_add(1);
		m_activeReadbackObjects[activeArrayIndex] = index;
		obj.setReadbackTarget(target);
		return RCObjectPtr<ReadbackObject>(&obj);
	}

	void ReadbackManager::freeResource(uint32_t index, ReadbackTarget target)
	{

	}
	void ReadbackManager::allocateResource(uint32_t index, ReadbackTarget target)
	{

	}
}