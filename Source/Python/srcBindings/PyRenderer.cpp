#include <PyRenderer.h>
#include <Renderer/Renderer.h>
#include <Renderer/RendererConfiguration.h>
#include <pybind11/stl.h>
#include <assert.h>
#include <PyRendererCache.h>
#include <PyTexture.h>
#include <PyBuffer.h>
#include <PyMaterial.h>
#include <PyReadback.h>

namespace YAPT
{

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


	PyTexture* PyRenderer::createTexture(const char* name, ResourceDimension dimensions, ResourceFormat format, ResourceUsage resourceUsage, uint32_t width, uint32_t height, uint32_t mips, uint32_t depthOrSlices)
	{
		Texture* tex = m_renderer->createTexture(name, dimensions, format, resourceUsage, width, height, mips, depthOrSlices);
		if (name != nullptr)
		{
			tex->setName(name);
		}
		PyTexture* pytex =  new PyTexture(tex);
		tex->Release();
		return pytex;
	}
	PyBuffer* PyRenderer::createBuffer(const char* name, ResourceUsage resourceUsage, size_t size)
	{
		Buffer* buff = m_renderer->createBuffer(name, resourceUsage, size);
		if (name != nullptr)
		{
			buff->setName(name);
		}
		PyBuffer* pybuf =  new PyBuffer(buff);
		buff->Release();
		return pybuf;
	}

	std::shared_ptr<PyMaterial> PyRenderer::createMaterial(const char* name)
	{
		return std::make_shared<PyMaterial>(m_renderer, name);
	}
	std::shared_ptr<PyMesh> PyRenderer::createMesh(const char* name, const std::vector<PyVertexBufferLayout>& layouts, size_t vertexCount, size_t submeshCount, bool use16BitIndices)
	{
		return std::make_shared<PyMesh>(m_renderer, name,  layouts, vertexCount, submeshCount, use16BitIndices);
	}

	std::shared_ptr<PyReadbackObject> PyRenderer::readback(ReadbackTarget target)
	{
		assert(m_renderer != nullptr);
		ReadbackHandle readback = m_renderer->readback(target);
		return std::make_shared<PyReadbackObject>(readback.get());
	}

	BINDING_FUNC(PyRenderer, m)
	{
		pybind11::class_<PyRenderer>(m, "Renderer")
			.def(pybind11::init<>())
			.def("init", &PyRenderer::init)
			.def("shutdown", &PyRenderer::shutdown)
			.def("getRendererVariable", &PyRenderer::getRendererVariable)
			.def("getAllRendererVariableNames", &PyRenderer::getAllRendererVariableNames)
			.def("setRenderOutputToSurface", &PyRenderer::setRenderOutputToSurface)
			.def("resetRenderOutput", &PyRenderer::resetRenderOutput)
			.def("createTexture", &PyRenderer::createTexture)
			.def("createBuffer", &PyRenderer::createBuffer)
			.def("createMesh", &PyRenderer::createMesh)
			.def("createMaterial", &PyRenderer::createMaterial)
			.def("readback", &PyRenderer::readback);
	
	}
}
