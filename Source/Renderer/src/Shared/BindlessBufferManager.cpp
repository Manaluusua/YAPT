#include <Renderer/Shared/BindlessBufferManager.h>
#include <Renderer/Shared/CRenderer.h>

#include <Renderer/Shared/TextureImpl.h>
namespace YAPT
{
	BindlessBufferManager::BindlessBufferManager(CRenderer* renderer)
		:m_renderer(renderer),
		m_indexAllocator(nullptr)
	{

	}
	BindlessBufferManager::~BindlessBufferManager()
	{
		deinit();
	}

	void BindlessBufferManager::init(uint32_t maxNumberOfBuffers)
	{
		deinit();

		m_indexAllocator = new DeferredFreeArrayIndexAllocator(m_renderer->getPipelineLength() + 1, maxNumberOfBuffers);

		DescriptorSetLayoutBinding b;
		b.accessFlags = ACCESS_FLAGS_READ;
		b.bindingIndex = 0;
		b.descriptorCount = maxNumberOfBuffers;
		b.shaderStages = ALL_SHADER_STAGES;
		b.staticSamplers = nullptr;
		b.type = DescriptorType::UNIFORM_TEXEL_BUFFER; //Buffer<> type in hlsl parlance
		m_resourceUtility.init(m_renderer->getGfxHandle(), maxNumberOfBuffers, b);
	}

	void BindlessBufferManager::deinit()
	{
		m_resourceUtility.deinit();
		if (m_indexAllocator)
		{
			delete m_indexAllocator;
			m_indexAllocator = nullptr;
		}
	}

	void BindlessBufferManager::flush()
	{
		m_indexAllocator->flush();
	}

	void BindlessBufferManager::bufferToBeCreated(BufferDesc& d)
	{
		d.resourceUsage |= RESOURCE_USAGE_UNIFORM_TEXEL_BUFFER;
	}

	void BindlessBufferManager::bufferCreated(BufferImpl* buff)
	{
		uint32_t index = m_indexAllocator->allocate();
		buff->setBindlessResourceArrayIndex(index);
		 
		BufferViewDesc desc;
		desc.structureStrideInBytes = 4; //for now always use 4 byte alignment
		BufferViewHandle handle = Gfx::getBufferView(m_renderer->getGfxHandle(), buff->getResourceHandle(), desc);

		m_resourceUtility.updateDescriptor(index, handle);

	}	
	
	
	void BindlessBufferManager::bufferReleased(BufferImpl* buff)
	{
		m_indexAllocator->free(buff->getBindlessResourceArrayIndex());
		buff->setBindlessResourceArrayIndex(uint32_t(-1));
	}

}