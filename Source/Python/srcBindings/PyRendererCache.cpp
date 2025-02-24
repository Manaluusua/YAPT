#include <PyRendererCache.h>
#include <AssetLoader/AssetLoader.h>
namespace YAPT
{
	DEFINE_BINDING_CLASS(PyRendererCache);

	PyRendererCache* PyRendererCache::getDefaultRendererCache()
	{
		return new PyRendererCache(getDefaultRendererCacheProvider());
	}

	PyRendererCache::PyRendererCache(RendererCacheProvider* cache)
		:m_rendererCache(cache)
	{

	}
	PyRendererCache::~PyRendererCache()
	{

	}

	RendererCacheProvider* PyRendererCache::getRendererCache()
	{
		return m_rendererCache;
	}

	BINDING_FUNC(PyRendererCache, m)
	{
		pybind11::class_<PyRendererCache>(m, "RendererCache")
			.def_static("getDefaultRendererCache", &PyRendererCache::getDefaultRendererCache);
	}

}