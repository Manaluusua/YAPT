#include <PyRenderer.h>
#include <Renderer/Renderer.h>
#include <Renderer/RendererConfiguration.h>
#include <pybind11/stl.h>
#include <assert.h>
#include <PyRendererCache.h>
#include <PyTexture.h>
#include <PyBuffer.h>

namespace YAPT
{
	/////////////////////// PyAllocationPool ///////////////////////

	PyAllocationPool::PyAllocationPool(ResourceAllocationPool* pool)
		:m_allocPool(pool)
	{

	}

	PyTexture* PyAllocationPool::addTexture(const char* name, ResourceDimension dimensions, ResourceFormat format, ResourceUsage resourceUsage, uint32_t width, uint32_t height, uint32_t mips = 1, uint32_t depthOrSlices = 1)
	{
		Texture* tex = m_allocPool->addTexture(name, dimensions, format, resourceUsage, width, height, mips, depthOrSlices);
		return new PyTexture(tex);
	}

	PyBuffer* PyAllocationPool::addBuffer(const char* name, ResourceUsage resourceUsage, size_t size)
	{
		Buffer* b = m_allocPool->addBuffer(name, resourceUsage, size);
		return new PyBuffer(b);
	}

	void PyAllocationPool::allocateAndConsume()
	{
		m_allocPool->allocateAndConsume();
		m_allocPool = nullptr;
	}

	/////////////////////// PyRenderer ///////////////////////

	DEFINE_BINDING_CLASS(PyRenderer);

	PyRenderer::PyRenderer()
		:m_renderer(nullptr)
	{
	}
	PyRenderer::~PyRenderer()
	{

	}

	void PyRenderer::init(PyRendererCache* cache, uintptr_t windowHandle, bool enableGPUTrace)
	{
		assert(m_renderer == nullptr);
		m_renderer = YAPT::createRenderer();

		RendererInitializeConfig config;
		config.cache = cache->getRendererCache();
		config.renderSurfaceHandle = (HWND)windowHandle;
		config.enableGPUDebugCapture = enableGPUTrace;

		m_renderer->initialize(config);
	}
	void PyRenderer::release()
	{
		if (m_renderer)
		{
			YAPT::destroyRenderer(m_renderer);
			m_renderer = nullptr;
		}
	}

	PyAllocationPool* PyRenderer::createAllocationPool()
	{
		ResourceAllocationPool* pool = m_renderer->createResourceAllocationPool();
		return new PyAllocationPool(pool);
	}

	PyRendererVar PyRenderer::getRendererVariable(const char* varName)
	{
		assert(m_renderer != nullptr);
		return PyRendererVar(m_renderer->getRendererConfiguration()->getRendererVariable(varName));
	}

	std::vector<const char*> PyRenderer::getAllRendererVariableNames()
	{
		assert(m_renderer != nullptr);
		size_t rendererVarsCount = m_renderer->getRendererConfiguration()->getNumberOfRendererVariables();
		std::vector<const char*> rendererVars;
		rendererVars.resize(rendererVarsCount);
		m_renderer->getRendererConfiguration()->queryRendererVariableNamesList(rendererVars.data(), rendererVarsCount);
		return rendererVars;
	}


	BINDING_FUNC(PyRenderer, m)
	{

		pybind11::class_<PyRenderer>(m, "Renderer")
			.def(pybind11::init<>())
			.def("init", &PyRenderer::init)
			.def("release", &PyRenderer::release)
			.def("getRendererVariable", &PyRenderer::getRendererVariable)
			.def("getAllRendererVariableNames", &PyRenderer::getAllRendererVariableNames)
			.def("createAllocationPool", &PyRenderer::createAllocationPool);
	
		pybind11::class_<PyAllocationPool>(m, "AllocationPool")
			.def("addTexture", &PyAllocationPool::addTexture)
			.def("addBuffer", &PyAllocationPool::addBuffer)
			.def("allocate", &PyAllocationPool::allocateAndConsume);
	}
}
