#include <PyBuffer.h>

namespace YAPT
{
	DEFINE_BINDING_CLASS(PyBuffer)

	PyBuffer::PyBuffer(Buffer* buffer)
		:m_buffer(buffer),
		m_memoryAllocated(false)
	{
	}
	PyBuffer::~PyBuffer()
	{
		m_buffer = nullptr;
	}

	void PyBuffer::upload(size_t offsetInBytes, size_t sizeInBytes, const void* data)
	{

	}
	void PyBuffer::memoryAllocated()
	{
		m_memoryAllocated = true;
	}

	BINDING_FUNC(PyBuffer, m)
	{
		pybind11::class_<PyBuffer>(m, "Buffer")
			.def("upload", &PyBuffer::upload);
	}
}