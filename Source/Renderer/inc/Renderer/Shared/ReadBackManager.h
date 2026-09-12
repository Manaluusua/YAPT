#pragma once
#include <Common/RCObject.h>
#include <Renderer/ReadbackHandle.h>
#include <Gfx/GfxTypes.h>
#include <Gfx/GfxApi.h>
#include <Common/DeferredFreeArrayIndexAllocator.h>
#include <atomic>

namespace YAPT
{
	class CRenderer;
	class ReadbackManager;
	class RenderPipelineManager;

	class ReadbackGroup : public RCObject
	{
	public:
		ReadbackGroup(GfxApiHandle gfx, FenceHandle fence)
			:m_gfx(gfx),
			m_fence(fence)
		{

		}

		~ReadbackGroup()
		{
			Gfx::freeFence(m_gfx, m_fence);
		}
		FenceHandle fenceHandle = InvalidFenceHandle;

	private:
		GfxApiHandle m_gfx;
		FenceHandle m_fence;
	};

	class CReadbackObject : public ReadbackObject
	{
	public:
		friend class ReadbackManager;
		CReadbackObject()
			:m_state(ReadbackState::Freed),
			m_index(uint32_t(-1)),
			_buffHandle(YAPT_NULL_HANDLE),
			_texHandle(YAPT_NULL_HANDLE)
		{

		}

		virtual ReadbackState getState() final
		{
			return m_state.load();
		}

		virtual void allReferencesReleased() final
		{
			setState(ReadbackState::Freed);
		}
	private:

		virtual ReadbackState getStateNonAtomic() final
		{
			return m_state;
		}

		void setReadbackTarget(ReadbackTarget target)
		{
			m_target = target;
		}

		ReadbackTarget getReadbackTarget() const
		{
			return m_target;
		}

		void setState(ReadbackState state)
		{
			if (state == ReadbackState::Ready)
			{
				while (m_state.load() == ReadbackState::Pending)
				{
					ReadbackState expected = ReadbackState::Pending;
					if (m_state.compare_exchange_weak(expected, ReadbackState::Ready, std::memory_order_acq_rel))
					{
						break;
					}
				}
			}
			else
			{
				m_state.store(state);
			}
			
		}

		void setIndex(uint32_t index)
		{
			m_index = index;
		}
		uint32_t getIndex() const
		{
			return m_index;
		}

		
		std::atomic<ReadbackState> m_state;
		uint32_t m_index;
		ReadbackTarget m_target;
		//Manager by ReadbackManager
		TextureHandle _texHandle;
		BufferHandle _buffHandle;
		RCObjectPtr<ReadbackGroup> _readbackGroup;
	};

	class ReadbackManager 
	{
		friend class CReadbackObject;
	public:
		ReadbackManager(CRenderer& renderer);
		~ReadbackManager();

		void commitChanges();

		void onBeforeRender(RenderPipelineManager* mngr);
		void onAfterRender(RenderPipelineManager* mngr);

		ReadbackHandle readback(ReadbackTarget target);
	private:
		

		static constexpr uint32_t MAX_READBACK_REQUESTS = 256;

		void issueReadback(CReadbackObject& obj, TextureHandle src, TextureHandle target, ReadbackGroup* fence, TextureReadbackDefinition& defOut);
		void issueReadback(CReadbackObject& obj, BufferHandle src, BufferHandle target, ReadbackGroup* fence, BufferReadbackDefinition& defOut);

		TextureHandle getMatchingResource(TextureHandle handle);
		BufferHandle getMatchingResource(BufferHandle handle);

		void freeResource(uint32_t index, ReadbackTarget target);

		

		CRenderer& m_renderer;
		DeferredFreeArrayIndexAllocator m_freeReadbackObjects;
		CReadbackObject m_readbackObjectPool[MAX_READBACK_REQUESTS];
		uint32_t m_activeReadbackObjects[MAX_READBACK_REQUESTS];
		uint32_t m_newReadbackRequests[MAX_READBACK_REQUESTS];
		std::atomic<uint32_t> m_activeReadbackObjectCount;
		uint32_t m_newReadbackRequestsCount;
		



	};

}