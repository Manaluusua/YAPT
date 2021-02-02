#pragma once

#include <Renderer/Shared/Utility/BindlessResourceUtility.h>
#include <Common/DeferredFreeArrayIndexAllocator.h>
namespace YAPT
{
	class CRenderer;
	class TextureImpl;
	class BindlessTextureManager
	{
	public:
		BindlessTextureManager(CRenderer* renderer);
		~BindlessTextureManager();

		void init(uint32_t maxNumberOfTextures);
		void deinit();

		void flush();

		void textureCreated(TextureImpl* tex);
		void textureReleased(TextureImpl* tex);

		uint32_t addTextureView(TextureViewHandle texView);
		void removeTextureView(uint32_t index);

		DescriptorSetLayoutHandle getLayout() const { return m_resourceUtility.getLayout(); }
		const DescriptorSetLayoutBinding& getBindingDefinition() const { return m_resourceUtility.getBindingDefinition(); }
		DescriptorSetHandle getTextureArrayDescSet() const { return m_resourceUtility.getResourceArrayDescSet(); }

	private:
		CRenderer* m_renderer;
		BindlessResourceUtility m_resourceUtility;
		DeferredFreeArrayIndexAllocator* m_indexAllocator;
	};
}