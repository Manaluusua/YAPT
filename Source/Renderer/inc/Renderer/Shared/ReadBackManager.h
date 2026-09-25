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
			:m_fenceHandle(fence),
			m_gfx(gfx),
			m_lastQueriedState(FenceState::Unused)
		{

		}

		FenceState getFenceState()
		{
			FenceState lastState = m_lastQueriedState.load();
			if (lastState == FenceState::Pending)
			{
				FenceState state = Gfx::getFenceState(m_gfx, m_fenceHandle);
				if (state != FenceState::Pending)
				{
					m_lastQueriedState.store(state);
					lastState = state;
				}
			}
			return lastState;
		}

		~ReadbackGroup()
		{
			Gfx::freeFence(m_gfx, m_fenceHandle);
		}
		
		FenceHandle getFenceHandle() const { return m_fenceHandle; }

	private:
		GfxApiHandle m_gfx;
		FenceHandle m_fenceHandle = InvalidFenceHandle;
		std::atomic<FenceState> m_lastQueriedState;
	};

	class CReadbackObject : public ReadbackObject
	{
	public:
		friend class ReadbackManager;
		CReadbackObject()
			:m_state(ReadbackState::Freed),
			m_index(uint32_t(-1)),
			m_buffHandle(YAPT_NULL_HANDLE),
			m_texHandle(YAPT_NULL_HANDLE)
		{

		}

		void init(GfxApiHandle gfx)
		{
			m_gfxHandle = gfx;
			m_readbackData.data = nullptr;
		}

		virtual ReadbackState getState() final
		{
			ReadbackState state = m_state.load();
			if (state == ReadbackState::Pending)
			{
				if (_readbackGroup != nullptr)
				{
					FenceState fenceState =_readbackGroup->getFenceState();
					if (fenceState != FenceState::Pending)
					{
						setState(ReadbackState::Ready);
						state = ReadbackState::Ready;
					}
				}
			}
			return state;
		}

		virtual const ReadbackData* getData() final
		{
			if (getState() != ReadbackState::Ready)
			{
				return nullptr;
			}

			if (m_readbackData.data == nullptr)
			{
				if (m_texHandle != YAPT_NULL_HANDLE)
				{
					m_readbackData.data = Gfx::map(m_gfxHandle, m_texHandle, 0, 0);
				} else
				{
					assert(m_buffHandle != YAPT_NULL_HANDLE);
					m_readbackData.data = Gfx::map(m_gfxHandle, m_buffHandle, 0, m_readbackData.widthOrSizeInBytes);
				}

				
			}
			return &m_readbackData;
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
			ReadbackState currentState = m_state.load();
			if (state == ReadbackState::Ready && currentState == ReadbackState::Pending)
			{
				while (currentState == ReadbackState::Pending)
				{
					ReadbackState expected = ReadbackState::Pending;
					if (m_state.compare_exchange_weak(expected, ReadbackState::Ready, std::memory_order_acq_rel))
					{
						break;
					}
					currentState = m_state.load();
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

		void setResource(TextureHandle resource)
		{
			m_texHandle = resource;
			assert(m_buffHandle == YAPT_NULL_HANDLE);
			const TextureDesc& desc = Gfx::getDesc(m_gfxHandle, resource);
			m_readbackData.dimensions = desc.dimension;
			m_readbackData.format = desc.format;
			m_readbackData.widthOrSizeInBytes = desc.width;
			m_readbackData.height = desc.height;
			m_readbackData.depthOrSlices = desc.depthOrSlices;
		}

		void setResource(BufferHandle resource)
		{
			m_buffHandle = resource;
			assert(m_texHandle == YAPT_NULL_HANDLE);

			const BufferDesc& desc = Gfx::getDesc(m_gfxHandle, resource);
			m_readbackData.dimensions = ResourceDimension::BUFFER;
			m_readbackData.format = ResourceFormat::UNKNOWN;
			m_readbackData.widthOrSizeInBytes = desc.sizeInBytes;
			m_readbackData.height = 0;
			m_readbackData.depthOrSlices = 0;
		}

		void clearResource()
		{
			//TODO: pool these and return to pool

			if (m_texHandle != YAPT_NULL_HANDLE)
			{
				if (m_readbackData.data != nullptr)
				{
					Gfx::unmap(m_gfxHandle, m_texHandle, 0, 0);
				}
				Gfx::destroyTexture(m_gfxHandle, m_texHandle);
				m_texHandle = YAPT_NULL_HANDLE;
			}


			if (m_buffHandle != YAPT_NULL_HANDLE)
			{
				if (m_readbackData.data != nullptr)
				{
					Gfx::unmap(m_gfxHandle, m_buffHandle, 0, m_readbackData.widthOrSizeInBytes);
				}
				Gfx::destroyBuffer(m_gfxHandle, m_buffHandle);
				m_buffHandle = YAPT_NULL_HANDLE;
			}
			m_readbackData.data = nullptr;
		}
		
		std::atomic<ReadbackState> m_state;
		uint32_t m_index;
		ReadbackTarget m_target;
		TextureHandle m_texHandle;
		BufferHandle m_buffHandle;
		GfxApiHandle m_gfxHandle;
		ReadbackData m_readbackData;
		//Manager by ReadbackManager
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