#pragma once
#include <PyBindingsCommon.h>
#include <memory>

namespace YAPT
{
	class RendererCacheProvider;
	class PyRendererCache
	{
	public:
		DECLARE_BINDING_CLASS(PyRendererCache);
		PyRendererCache(RendererCacheProvider* cache);
		~PyRendererCache();

		RendererCacheProvider* getRendererCache();

		static PyRendererCache* getDefaultRendererCache();
	private:
		RendererCacheProvider* m_rendererCache;

	};
}

