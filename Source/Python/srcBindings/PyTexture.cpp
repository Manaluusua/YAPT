#include <PyTexture.h>

namespace YAPT
{
	DEFINE_BINDING_CLASS(PyTexture)

	PyTexture::PyTexture(Texture* t)
		:m_texture(t),
		m_memoryAllocated(false)
	{
	}
	PyTexture::~PyTexture()
	{
		m_texture = nullptr;
	}

	void PyTexture::upload(uint32_t mipOffset, uint32_t arrayOffset, uint32_t mipCount, uint32_t arrayCount, size_t rowPitchInBytes, const void* data)
	{

	}

	void PyTexture::memoryAllocated()
	{
		m_memoryAllocated = true;
	}

	BINDING_FUNC(PyTexture, m)
	{
		pybind11::class_<PyTexture>(m, "Texture")
			.def("upload", &PyTexture::upload);
	}
}