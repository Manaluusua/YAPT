#include <Renderer/Shared/BindlessTextureManager.h>
#include <Renderer/Shared/CRenderer.h>

#include <Renderer/Shared/TextureImpl.h>
namespace YAPT
{
	BindlessTextureManager::BindlessTextureManager(CRenderer* renderer)
		:m_renderer(renderer),
		m_indexAllocator(nullptr)
	{

	}
	BindlessTextureManager::~BindlessTextureManager()
	{
		deinit();
	}

	void BindlessTextureManager::init(uint32_t maxNumberOfTextures)
	{
		deinit();

		m_indexAllocator = new DeferredFreeArrayIndexAllocator(m_renderer->getPipelineLength() + 1, maxNumberOfTextures);

		DescriptorSetLayoutBinding b;
		b.accessFlags = ACCESS_FLAGS_READ;
		b.bindingIndex = 0;
		b.descriptorCount = maxNumberOfTextures;
		b.shaderStages = ALL_SHADER_STAGES;
		b.staticSamplers = nullptr;
		b.type = DescriptorType::TEXTURE;
		m_resourceUtility.init(m_renderer->getGfxHandle(), maxNumberOfTextures, b);
	}

	void BindlessTextureManager::deinit()
	{
		m_resourceUtility.deinit();
		if (m_indexAllocator)
		{
			delete m_indexAllocator;
			m_indexAllocator = nullptr;
		}
	}

	void BindlessTextureManager::flush()
	{
		m_indexAllocator->flush();
		m_resourceUtility.flush();
	}

	void BindlessTextureManager::textureToBeCreated(TextureDesc& d)
	{

	}

	void BindlessTextureManager::textureCreated(TextureImpl* tex)
	{
		TextureViewDesc desc;
		desc.format = tex->getDesc().format;
		desc.resourceUsage = RESOURCE_USAGE_SAMPLED_TEXTURE;
		TextureViewHandle handle = Gfx::getTextureView(m_renderer->getGfxHandle(), tex->getResourceHandle(), desc);
		uint32_t index = addTextureView(handle);

		tex->setBindlessResourceArrayIndex(index);
	}	
	
	uint32_t BindlessTextureManager::addTextureView(TextureViewHandle texView)
	{
		uint32_t index = m_indexAllocator->allocate();
		m_resourceUtility.updateDescriptor(index, texView);
		return index;
	}
	void BindlessTextureManager::removeTextureView(uint32_t index)
	{
		m_indexAllocator->free(index);
	}
	
	void BindlessTextureManager::textureReleased(TextureImpl* tex)
	{
		removeTextureView(tex->getBindlessResourceArrayIndex());
		tex->setBindlessResourceArrayIndex(uint32_t(-1));
	}

}