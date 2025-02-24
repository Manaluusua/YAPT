#pragma once
#include <PyBindingsCommon.h>
#include <PyRendererVar.h>
#include <Windows.h>

namespace YAPT
{
	class Renderer;
	class PyRendererVar;
	class PyRendererCache;

	class PyRenderer
	{
	public:
		DECLARE_BINDING_CLASS(PyRenderer);

		PyRenderer();
		~PyRenderer();

		void init(PyRendererCache* cache, uintptr_t windowHandle, bool enableGPUTrace);
		void release();
		
		PyRendererVar getRendererVariable(const char* varName);
		std::vector<const char*> getAllRendererVariableNames();

		inline Renderer* getRenderer() { return m_renderer; }
	private:
		Renderer* m_renderer;

		
	};
}

