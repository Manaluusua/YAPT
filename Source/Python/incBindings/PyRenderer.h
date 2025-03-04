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


	
	class PyRenderer
	{
	public:
		DECLARE_BINDING_CLASS(PyRenderer);

		PyRenderer();
		~PyRenderer();

		void init(PyRendererCache* cache, uintptr_t windowHandle, bool enableGPUTrace);
		void shutdown();
		
		void setRenderOutputToSurface(size_t width, size_t height, uintptr_t windowHandle);
		void resetRenderOutput();

		

		PyRendererVar getRendererVariable(const char* varName);
		std::vector<const char*> getAllRendererVariableNames();

		inline Renderer* getRenderer() { return m_renderer; }
	private:
		Renderer* m_renderer;

		
	};
}

