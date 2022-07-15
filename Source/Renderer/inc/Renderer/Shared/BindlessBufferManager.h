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

		void bufferToBeCreated(BufferDesc& d);
		void bufferCreated(BufferImpl* b);
		void bufferReleased(BufferImpl* b);

		DescriptorSetLayoutHandle getLayout() const { return m_resourceUtility.getLayout(); }
		const DescriptorSetLayoutBinding& getBindingDefinition() const { return m_resourceUtility.getBindingDefinition(); }
		DescriptorSetHandle getBufferArrayDescSet() const { return m_resourceUtility.getResourceArrayDescSet(); }

	private:
		CRenderer* m_renderer;
		BindlessResourceUtility m_resourceUtility;
		DeferredFreeArrayIndexAllocator* m_indexAllocator;
	};
}