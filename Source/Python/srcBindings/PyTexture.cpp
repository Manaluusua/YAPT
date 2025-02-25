#include <PyTexture.h>

namespace YAPT
{
	DEFINE_BINDING_CLASS(PyTexture)

	PyTexture::PyTexture(Texture* t)
	{
		m_texture = t;
	}
	PyTexture::~PyTexture()
	{
		m_texture = nullptr;
	}

	BINDING_FUNC(PyTexture, m)
	{

	}
}