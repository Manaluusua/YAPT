#include <PyTexture.h>

namespace YAPT
{
	DEFINE_BINDING_CLASS(PyTexture)

	PyTexture::PyTexture(Texture* t)
		:m_texture(t)
	{
	}
	PyTexture::~PyTexture()
	{
		m_texture = nullptr;
	}

	void PyTexture::upload(uint32_t mipOffset, uint32_t arrayOffset, uint32_t mipCount, uint32_t arrayCount, size_t rowPitchInBytes, uintptr_t data, size_t dataPtrOffset)
	{
		char* ptr = reinterpret_cast<char*>(data);
		TextureDataDefinition def;
		def.data = ptr + dataPtrOffset;
		def.rowPitchInBytes = rowPitchInBytes;
		m_texture->upload(mipOffset, arrayOffset, mipCount, arrayCount, &def);
	}

	const char* PyTexture::getName() const
	{
		return m_texture->getName();
	}

	BINDING_FUNC(PyTexture, m)
	{
		pybind11::class_<PyTexture>(m, "Texture")
			.def("upload", &PyTexture::upload)
			.def("getName", &PyTexture::getName);
	}
}