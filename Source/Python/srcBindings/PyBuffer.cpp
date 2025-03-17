#include <PyBuffer.h>

namespace YAPT
{
	DEFINE_BINDING_CLASS(PyBuffer)

	PyBuffer::PyBuffer(Buffer* buffer)
		:m_buffer(buffer)
	{
	}
	PyBuffer::~PyBuffer()
	{
		m_buffer = nullptr;
	}

	void PyBuffer::upload(size_t offsetInBytes, size_t sizeInBytes, uintptr_t data, size_t dataPtrOffset)
	{
		char* ptr = reinterpret_cast<char*>(data) + dataPtrOffset;
		m_buffer->upload(offsetInBytes, sizeInBytes, ptr);
	}

	const char* PyBuffer::getName() const
	{
		return m_buffer->getName();
	}

	BINDING_FUNC(PyBuffer, m)
	{
		pybind11::class_<PyBuffer>(m, "Buffer")
			.def("upload", &PyBuffer::upload)
			.def("getName", &PyBuffer::getName);
	}
}