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
		for (size_t i = 0; i < MAX_READBACK_REQUESTS; ++i)
		{
			m_readbackObjectPool[i].init(m_renderer.getGfxHandle());
		}
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
				break;
				default:
					assert(!"unknown readback target");
				}
			}


			if (!isValidRequest)
			{
				obj.setState(ReadbackState::Failed);
			}
		}
		m_newReadbackRequestsCount = 0;

		if (textureReadbackDefs.size() > 0 || bufferReadbackDefs.size() > 0)
		{
			assert(group != nullptr);
			readbackDefs.textureReadbackDefinitions = textureReadbackDefs.data();
			readbackDefs.textureReadbackCount = (uint32_t)textureReadbackDefs.size();
			readbackDefs.bufferReadbackDefinitions = bufferReadbackDefs.data();
			readbackDefs.bufferReadbackCount = (uint32_t)bufferReadbackDefs.size();

			Gfx::readback(m_renderer.getGfxHandle(), readbackDefs, GpuDownloadStage::AFTER_RENDER, group->getFenceHandle());
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
		obj.clearResource();
		obj._readbackGroup = nullptr;
	}

	TextureHandle ReadbackManager::getMatchingResource(TextureHandle handle)
	{
		TextureDesc desc = Gfx::getDesc(m_renderer.getGfxHandle(), handle);
		desc.memoryType = MemoryType::CPU_MAPPABLE_READBACK;
		desc.resourceUsage |= RESOURCE_USAGE_COPY_DESTINATION; 
		//TODO: pool
		return Gfx::createTexture(m_renderer.getGfxHandle(), desc, ResourceStateDescription::defaultInitialResourceState(), "readbackTexture");
		
	}
	BufferHandle ReadbackManager::getMatchingResource(BufferHandle handle)
	{
		BufferDesc desc = Gfx::getDesc(m_renderer.getGfxHandle(), handle);
		desc.memoryType = MemoryType::CPU_MAPPABLE_READBACK;
		desc.resourceUsage |= RESOURCE_USAGE_COPY_DESTINATION;
		//TODO: pool
		return Gfx::createBuffer(m_renderer.getGfxHandle(), desc, ResourceStateDescription::defaultInitialResourceState(), "readbackBuffer");
	}

	void ReadbackManager::issueReadback(CReadbackObject& obj, TextureHandle src, TextureHandle target, ReadbackGroup* group, TextureReadbackDefinition& defOut)
	{

		const TextureDesc& desc = Gfx::getDesc(m_renderer.getGfxHandle(), src);
		const bool isVolume = desc.dimension == ResourceDimension::TEXTURE_3D;

		ImageCopySubresourceDefinition subresource;
		subresource.mip = 0;
		subresource.arraySliceOffset = 0;
		subresource.arraySliceCount = isVolume ? 1 : desc.depthOrSlices;

		defOut.src = src;
		defOut.dst = target;
		defOut.def.srcSubresource = subresource;
		defOut.def.srcOffset = { 0, 0, 0 };
		defOut.def.dstSubresource = subresource;
		defOut.def.dstOffset = { 0, 0, 0 };
		defOut.def.extent = { desc.width, desc.height, isVolume ? desc.depthOrSlices : 1 };

		obj.setResource(target);
		obj._readbackGroup = group;
		obj.setState(ReadbackState::Pending);
	}
	void ReadbackManager::issueReadback(CReadbackObject& obj, BufferHandle src, BufferHandle target, ReadbackGroup* group, BufferReadbackDefinition& defOut)
	{
		const BufferDesc& desc = Gfx::getDesc(m_renderer.getGfxHandle(), src);

		defOut.src = src;
		defOut.dst = target;
		defOut.def.srcOffset = 0;
		defOut.def.dstOffset = 0;
		defOut.def.size = desc.sizeInBytes;

		obj.setResource(target);
		obj._readbackGroup = group;
		obj.setState(ReadbackState::Pending);
	}
}