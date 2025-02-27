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
		PyTexture* pyTex = new PyTexture(tex);
		m_createdTextures.push_back(pyTex);
		return pyTex;
	}

	PyBuffer* PyAllocationPool::addBuffer(const char* name, ResourceUsage resourceUsage, size_t size)
	{
		Buffer* b = m_allocPool->addBuffer(name, resourceUsage, size);
		PyBuffer* pyBuf = new PyBuffer(b);
		m_createdBuffers.push_back(pyBuf);
		return pyBuf;
	}

	void PyAllocationPool::allocateAndConsume()
	{
		m_allocPool->allocateAndConsume();

		for (PyTexture* tex : m_createdTextures)
		{
			tex->memoryAllocated();
		}
		for (PyBuffer* b : m_createdBuffers)
		{
			b->memoryAllocated();
		}

		m_createdTextures.clear();
		m_createdTextures.clear();
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
		if (m_renderer)
		{
			shutdown();
		}
		m_renderer = YAPT::createRenderer();

		RendererInitializeConfig config;
		config.cache = cache->getRendererCache();
		config.renderSurfaceHandle = (HWND)windowHandle;
		config.enableGPUDebugCapture = enableGPUTrace;

		m_renderer->initialize(config);
	}
	void PyRenderer::shutdown()
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

	void PyRenderer::setRenderOutputToSurface(size_t width, size_t height, uintptr_t windowHandle)
	{
		WindowSurfaceDefinition def;
		def.width = width;
		def.height = height;
		def.windowHandle = (HWND)windowHandle;
		m_renderer->setRenderOutputToSurface(def);
	}
	void PyRenderer::resetRenderOutput()
	{
		m_renderer->resetRenderOutput();
	}


	BINDING_FUNC(PyRenderer, m)
	{
		pybind11::class_<PyRenderer>(m, "Renderer")
			.def(pybind11::init<>())
			.def("init", &PyRenderer::init)
			.def("shutdown", &PyRenderer::shutdown)
			.def("getRendererVariable", &PyRenderer::getRendererVariable)
			.def("getAllRendererVariableNames", &PyRenderer::getAllRendererVariableNames)
			.def("createAllocationPool", &PyRenderer::createAllocationPool)
			.def("setRenderOutputToSurface", &PyRenderer::setRenderOutputToSurface)
			.def("resetRenderOutput", &PyRenderer::resetRenderOutput);
	
		pybind11::class_<PyAllocationPool>(m, "AllocationPool")
			.def("addTexture", &PyAllocationPool::addTexture)
			.def("addBuffer", &PyAllocationPool::addBuffer)
			.def("allocate", &PyAllocationPool::allocateAndConsume);
	}
}
