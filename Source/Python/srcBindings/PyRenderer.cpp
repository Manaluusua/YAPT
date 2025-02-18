#include <PyRenderer.h>
#include <Renderer/Renderer.h>
#include <assert.h>

namespace YAPT
{
	DEFINE_BINDING_CLASS(PyRenderer);

	PyRenderer::PyRenderer()
		:m_renderer(nullptr)
	{
	}
	PyRenderer::~PyRenderer()
	{



	}

	void PyRenderer::init()
	{
		assert(m_renderer == nullptr);
		m_renderer = YAPT::createRenderer();
	}
	void PyRenderer::release()
	{
		if (m_renderer)
		{
			YAPT::destroyRenderer(m_renderer);
			m_renderer = nullptr;
		}
	}

	BINDING_FUNC(PyRenderer, m)
	{

		pybind11::class_<PyRenderer>(m, "Renderer")
			.def(pybind11::init<>())
			.def("init", &PyRenderer::init)
			.def("release", &PyRenderer::release);
	}
}
