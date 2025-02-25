#include <PyBuffer.h>

namespace YAPT
{
	DEFINE_BINDING_CLASS(PyBuffer)

	PyBuffer::PyBuffer(Buffer* buffer)
	{
		m_buffer = buffer;
	}
	PyBuffer::~PyBuffer()
	{
		m_buffer = nullptr;
	}

	BINDING_FUNC(PyBuffer, m)
	{

	}
}