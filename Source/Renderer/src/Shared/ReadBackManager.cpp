#include <Renderer/Shared/ReadBackManager.h>
#include <Renderer/ReadbackHandle.h>
#include <Renderer/Shared/CRenderer.h>
#include <Renderer/Shared/RenderPipeline/RenderPipelineManager.h>


namespace YAPT
{
	class ReadbackManager;
	

	ReadbackManager::ReadbackManager(CRenderer& renderer)
		:m_renderer(renderer),
		m_freeReadbackObjects(m_renderer.getPipelineLength(), MAX_READBACK_REQUESTS),
		m_activeReadbackObjectCount(0),
		m_newReadbackRequestsCount(0)
	{

	}
	ReadbackManager::~ReadbackManager()
	{

	}

	void ReadbackManager::onBeforeRender(RenderPipelineManager* mngr)
	{
		if (!mngr) return;
		for (uint32_t i = 0; i < m_newReadbackRequestsCount; ++i)
		{
			uint32_t reqIndex = m_newReadbackRequests[i];
			mngr->readbackRequested(m_readbackObjectPool[reqIndex].getReadbackTarget());
		}
	}
	void ReadbackManager::onAfterRender(RenderPipelineManager* mngr)
	{
		ReadbackDefinitions readbackDefs;
		std::vector<TextureReadbackDefinition> textureReadbackDefs;
		std::vector<BufferReadbackDefinition> bufferReadbackDefs;

		ReadbackGroup* group = nullptr;

		for (uint32_t i = 0; i < m_newReadbackRequestsCount; ++i)
		{
			uint32_t reqIndex = m_newReadbackRequests[i];
			CReadbackObject& obj = m_readbackObjectPool[reqIndex];
			bool isValidRequest = false;
			if (mngr)
			{
				switch (obj.getReadbackTarget())
				{
				case ReadbackTarget::FinalColor:
				{
					TextureHandle tex = mngr->getReadbackTextureResource(ReadbackTarget::FinalColor);
					if (tex != YAPT_NULL_HANDLE)
					{
						if (group == nullptr)
						{
							group = new ReadbackGroup(m_renderer.getGfxHandle(), Gfx::acquireFence(m_renderer.getGfxHandle(), FenceType::CPU_GPU_SYNC));
						}
						textureReadbackDefs.resize(textureReadbackDefs.size() + 1);
						issueReadback(obj, tex, getMatchingResource(tex), group, textureReadbackDefs.back());
						isValidRequest = true;
					}
				}
				default:
					assert(!"unknown readback target");
				}
			}
			

			if (!isValidRequest)
			{
				obj.setState(ReadbackState::Ready);
			}
		}
		m_newReadbackRequestsCount = 0;

		if (textureReadbackDefs.size() > 0 || bufferReadbackDefs.size() > 0)
		{
			Gfx::readback(m_renderer.getGfxHandle(), readbackDefs, GpuDownloadStage::AFTER_RENDER, group->fenceHandle);
		}
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
				m_newReadbackRequests[m_newReadbackRequestsCount] = index;
				++m_newReadbackRequestsCount;
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
		obj.setState(ReadbackState::Created);
		uint32_t activeArrayIndex = m_activeReadbackObjectCount.fetch_add(1);
		m_activeReadbackObjects[activeArrayIndex] = index;
		obj.setReadbackTarget(target);
		return RCObjectPtr<ReadbackObject>(&obj);
	}

	void ReadbackManager::freeResource(uint32_t index, ReadbackTarget target)
	{
		CReadbackObject& obj = m_readbackObjectPool[index];
		if (obj._texHandle != YAPT_NULL_HANDLE)
		{
			Gfx::destroyTexture(m_renderer.getGfxHandle(), obj._texHandle);
			obj._texHandle = YAPT_NULL_HANDLE;
		}
		if (obj._buffHandle != YAPT_NULL_HANDLE)
		{
			Gfx::destroyBuffer(m_renderer.getGfxHandle(), obj._buffHandle);
			obj._buffHandle = YAPT_NULL_HANDLE;
		}
		obj._readbackGroup = nullptr;
	}

	TextureHandle ReadbackManager::getMatchingResource(TextureHandle handle)
	{
		TextureDesc desc = Gfx::getDesc(m_renderer.getGfxHandle(), handle);
		desc.memoryType = MemoryType::CPU_MAPPABLE_READBACK;
		//TODO: pool
		return Gfx::createTexture(m_renderer.getGfxHandle(), desc, ResourceStateDescription::defaultInitialResourceState(), "readbackTexture");
		
	}
	BufferHandle ReadbackManager::getMatchingResource(BufferHandle handle)
	{
		BufferDesc desc = Gfx::getDesc(m_renderer.getGfxHandle(), handle);
		desc.memoryType = MemoryType::CPU_MAPPABLE_READBACK;
		//TODO: pool
		return Gfx::createBuffer(m_renderer.getGfxHandle(), desc, ResourceStateDescription::defaultInitialResourceState(), "readbackBuffer");
	}

	void ReadbackManager::issueReadback(CReadbackObject& obj, TextureHandle src, TextureHandle target, ReadbackGroup* group, TextureReadbackDefinition& defOut)
	{
		obj._texHandle = target;
		obj._buffHandle = YAPT_NULL_HANDLE;
		obj._readbackGroup = group;
		obj.setState(ReadbackState::Pending);
	}
	void ReadbackManager::issueReadback(CReadbackObject& obj, BufferHandle src, BufferHandle target, ReadbackGroup* group, BufferReadbackDefinition& defOut)
	{
		obj._texHandle = YAPT_NULL_HANDLE;
		obj._buffHandle = target;
		obj._readbackGroup = group;
		obj.setState(ReadbackState::Pending);
	}
}