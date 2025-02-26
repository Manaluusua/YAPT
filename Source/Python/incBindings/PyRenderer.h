#pragma once
#include <PyBindingsCommon.h>
#include <PyRendererVar.h>
#include <Windows.h>

namespace YAPT
{
	class Renderer;
	class PyRendererVar;
	class PyRendererCache;
	class ResourceAllocationPool;
	class PyTexture;
	class PyBuffer;


	class PyAllocationPool
	{
	public:
		PyAllocationPool(ResourceAllocationPool* pool);

		PyTexture* addTexture(const char* name, ResourceDimension dimensions, ResourceFormat format, ResourceUsage resourceUsage, uint32_t width, uint32_t height, uint32_t mips, uint32_t depthOrSlices);
		PyBuffer* addBuffer(const char* name, ResourceUsage resourceUsage, size_t size);

		void allocateAndConsume();
	private:
		ResourceAllocationPool* m_allocPool;
		std::vector<PyTexture*> m_createdTextures;
		std::vector<PyBuffer*> m_createdBuffers;
	};

	class PyRenderer
	{
	public:
		DECLARE_BINDING_CLASS(PyRenderer);

		PyRenderer();
		~PyRenderer();

		void init(PyRendererCache* cache, uintptr_t windowHandle, bool enableGPUTrace);
		void shutdown();
		
		PyAllocationPool* createAllocationPool();

		PyRendererVar getRendererVariable(const char* varName);
		std::vector<const char*> getAllRendererVariableNames();

		inline Renderer* getRenderer() { return m_renderer; }
	private:
		Renderer* m_renderer;

		
	};
}

