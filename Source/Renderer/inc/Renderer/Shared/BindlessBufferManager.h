#pragma once

#include <Renderer/Shared/Utility/BindlessResourceUtility.h>
#include <Common/DeferredFreeArrayIndexAllocator.h>
namespace YAPT
{
	class CRenderer;
	class BufferImpl;
	class BindlessBufferManager
	{
	public:
		BindlessBufferManager(CRenderer* renderer);
		~BindlessBufferManager();

		void init(uint32_t maxNumberOfBuffers);
		void deinit();

		void flush();

		void bufferCreated(BufferImpl* tex);
		void bufferReleased(BufferImpl* tex);

		DescriptorSetLayoutHandle getLayout() const { return m_resourceUtility.getLayout(); }
		const DescriptorSetLayoutBinding& getBindingDefinition() const { return m_resourceUtility.getBindingDefinition(); }
		DescriptorSetHandle getBufferArrayDescSet() const { return m_resourceUtility.getResourceArrayDescSet(); }

	private:
		CRenderer* m_renderer;
		BindlessResourceUtility m_resourceUtility;
		DeferredFreeArrayIndexAllocator* m_indexAllocator;
	};
}