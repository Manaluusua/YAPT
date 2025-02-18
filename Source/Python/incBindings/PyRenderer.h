#pragma once
#include <PyBindingsCommon.h>

namespace YAPT
{
	class Renderer;
	class PyRenderer
	{
	public:
		DECLARE_BINDING_CLASS(PyRenderer);

		PyRenderer();
		~PyRenderer();

		void init();
		void release();

		inline Renderer* getRenderer() { return m_renderer; }
	private:
		Renderer* m_renderer;

		
	};
}

