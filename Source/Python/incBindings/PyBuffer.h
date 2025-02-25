#pragma once
#include <PyBindingsCommon.h>
#include <Renderer/Buffer.h>
#include <Common/RCObjectPtr.h>

namespace YAPT
{

	class PyBuffer
	{
	public:
		DECLARE_BINDING_CLASS(PyBuffer);
		PyBuffer(Buffer* buffer);
		~PyBuffer();
		
	private:
		RCObjectPtr<Buffer> m_buffer;


	};
}
