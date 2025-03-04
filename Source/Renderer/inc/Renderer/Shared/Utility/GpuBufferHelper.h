#pragma once

#include <Common/CommonUtilities.h>
#include <Gfx/GfxTypes.h>
#include <Renderer/Shared/Utility/RenderResourcesPool.h>
#include <assert.h>
namespace YAPT
{
	template<typename T, size_t ARRAY_COUNT = 1>
	class FixedSizeGpuBufferHelper
	{
	public:

		FixedSizeGpuBufferHelper(ResourceUsage usage = RESOURCE_USAGE_COPY_DESTINATION | RESOURCE_USAGE_UNIFORM_BUFFER)
			:m_usage(usage),
			m_bufferHandle(YAPT_NULL_HANDLE),
			m_bufferView(YAPT_NULL_HANDLE)
		{

		}

		void init(RenderResourcesPool* pool)
		{
			BufferDesc buffDesc;
			buffDesc.sizeInBytes = BufferSize;
			buffDesc.resourceUsage = m_usage;
			m_bufferHandle = pool->requestBuffer(buffDesc, { RESOURCE_USAGE_COPY_DESTINATION, ACCESS_FLAGS_WRITE, SHADERSTAGE_NONE });
			m_gfxHandle = pool->getGfxHandle();

		}

		T* getData() { return m_internalData; }

		BufferViewHandle* getViewPtr()
		{
			if (m_bufferView == YAPT_NULL_HANDLE)
			{
				initView();
			}
			return &m_bufferView;
		}


		BufferViewHandle getView()
		{
			return *getViewPtr();
		}


		void flush()
		{
			char* dst = (char*)Gfx::mapBuffer(m_gfxHandle, m_bufferHandle, 0, BufferSize, GpuUploadStage::DURING_RENDER);

			for (size_t i = 0; i < ARRAY_COUNT; ++i)
			{
				memcpy(dst, &m_internalData[i], sizeof(T));
				dst += EntrySize;
			}
			Gfx::unmapBuffer(m_gfxHandle, m_bufferHandle);
			
		}
	private:

		void initView()
		{
			size_t alignment = Gfx::getBufferMinimumAlignment(m_gfxHandle, m_usage);

			BufferViewDesc buffViewDesc;
			buffViewDesc.offsetInBytes = 0;
			buffViewDesc.sizeInBytes = BufferSize;//align((size_t)BufferSize, alignment);
			buffViewDesc.structureStrideInBytes = EntrySize;
			m_bufferView = Gfx::getBufferView(m_gfxHandle, m_bufferHandle, buffViewDesc);
		}

		enum { EntrySize = align(sizeof(T), sizeof(float) * 4), BufferSize = EntrySize * ARRAY_COUNT};
		T m_internalData[ARRAY_COUNT];
		ResourceUsage m_usage;
		BufferHandle m_bufferHandle;
		BufferViewHandle m_bufferView;
		GfxApiHandle m_gfxHandle;
	};

	template<typename T>
	class DynamicSizeGpuBufferHelper
	{
	public:

		DynamicSizeGpuBufferHelper(GfxApiHandle handle, ResourceUsage usage = RESOURCE_USAGE_COPY_DESTINATION | RESOURCE_USAGE_UNIFORM_BUFFER)
			:m_usage(usage),
			m_bufferHandle(YAPT_NULL_HANDLE),
			m_bufferView(YAPT_NULL_HANDLE),
			m_gfxHandle(handle),
			m_allocatedEntryCount(0)
		{
			m_alignedEntrySize = align(sizeof(T), Gfx::getBufferMinimumAlignment(m_gfxHandle, usage));
		}


		DynamicSizeGpuBufferHelper(ResourceUsage usage = RESOURCE_USAGE_COPY_DESTINATION | RESOURCE_USAGE_UNIFORM_BUFFER)
			:m_usage(usage),
			m_bufferHandle(YAPT_NULL_HANDLE),
			m_bufferView(YAPT_NULL_HANDLE),
			m_gfxHandle(YAPT_NULL_HANDLE),
			m_allocatedEntryCount(0)
		{
		}

		void init(GfxApiHandle handle)
		{
			free();
			m_gfxHandle = handle;
			m_alignedEntrySize = align(sizeof(T), Gfx::getBufferMinimumAlignment(m_gfxHandle, m_usage));
		}

		~DynamicSizeGpuBufferHelper()
		{
			free();
		}

		size_t getAllocatedEntryCount() const { return m_allocatedEntryCount; }
		size_t getAlignedEntrySize() const { return m_alignedEntrySize; }

		void allocate(size_t numberOfEntries, const char* name = "")
		{
			assert(m_gfxHandle != YAPT_NULL_HANDLE);
			free();

			BufferDesc desc;
			desc.resourceUsage = RESOURCE_USAGE_COPY_DESTINATION | RESOURCE_USAGE_UNIFORM_BUFFER;
			desc.sizeInBytes = numberOfEntries * m_alignedEntrySize;
			ResourceStateDescription state{ RESOURCE_USAGE_COPY_DESTINATION, ACCESS_FLAGS_WRITE, SHADERSTAGE_NONE };
			m_bufferHandle = Gfx::createBuffer(m_gfxHandle, desc, state, "PerObjectData");
			m_allocatedEntryCount = numberOfEntries;


			BufferViewDesc buffViewDesc;
			buffViewDesc.offsetInBytes = 0;
			buffViewDesc.sizeInBytes = YAPT_BUFFER_WHOLE_RESOURCE;
			buffViewDesc.structureStrideInBytes = m_alignedEntrySize;
			m_bufferView = Gfx::getBufferView(m_gfxHandle, m_bufferHandle, buffViewDesc);

		}

		void free()
		{
			if (m_allocatedEntryCount > 0)
			{
				Gfx::destroyBuffer(m_gfxHandle, m_bufferHandle);
				m_bufferView = YAPT_NULL_HANDLE;
			}

			m_allocatedEntryCount = 0;
		}

		BufferViewHandle getBufferViewHandle() const { return m_bufferView; }

		char* map(size_t entryOffset, size_t entryCount)
		{
			char* dst = (char*)Gfx::mapBuffer(m_gfxHandle, m_bufferHandle, entryOffset * m_alignedEntrySize, entryCount * m_alignedEntrySize, GpuUploadStage::DURING_RENDER);
			return dst;
		}

		void unmap()
		{
			Gfx::unmapBuffer(m_gfxHandle, m_bufferHandle);
		}


	private:

		

		ResourceUsage m_usage;
		BufferHandle m_bufferHandle;
		GfxApiHandle m_gfxHandle;
		BufferViewHandle m_bufferView;
		size_t m_alignedEntrySize;
		size_t m_allocatedEntryCount;
	};


}